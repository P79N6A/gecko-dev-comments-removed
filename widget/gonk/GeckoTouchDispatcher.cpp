
















#include "FrameMetrics.h"
#include "GeckoProfiler.h"
#include "GeckoTouchDispatcher.h"
#include "InputData.h"
#include "ProfilerMarkers.h"
#include "base/basictypes.h"
#include "gfxPrefs.h"
#include "libui/Input.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/TouchEvents.h"
#include "mozilla/dom/Touch.h"
#include "mozilla/layers/APZThreadUtils.h"
#include "mozilla/layers/CompositorParent.h"
#include "nsAppShell.h"
#include "nsDebug.h"
#include "nsThreadUtils.h"
#include "nsWindow.h"
#include <sys/types.h>
#include <unistd.h>
#include <utils/Timers.h>

#define LOG(args...)                                            \
  __android_log_print(ANDROID_LOG_INFO, "Gonk" , ## args)




namespace mozilla {


static const uint64_t kInputExpirationThresholdMs = 1000;

static StaticRefPtr<GeckoTouchDispatcher> sTouchDispatcher;

 GeckoTouchDispatcher*
GeckoTouchDispatcher::GetInstance()
{
  if (!sTouchDispatcher) {
    sTouchDispatcher = new GeckoTouchDispatcher();
    ClearOnShutdown(&sTouchDispatcher);
  }
  return sTouchDispatcher;
}

GeckoTouchDispatcher::GeckoTouchDispatcher()
  : mTouchQueueLock("GeckoTouchDispatcher::mTouchQueueLock")
  , mHavePendingTouchMoves(false)
  , mInflightNonMoveEvents(0)
  , mTouchEventsFiltered(false)
{
  
  
  
  
  MOZ_ASSERT(sTouchDispatcher == nullptr);
  MOZ_ASSERT(NS_IsMainThread());
  gfxPrefs::GetSingleton();

  mEnabledUniformityInfo = gfxPrefs::UniformityInfo();
  mResamplingEnabled = gfxPrefs::TouchResampling() &&
                       gfxPrefs::HardwareVsyncEnabled();
  mVsyncAdjust = TimeDuration::FromMilliseconds(gfxPrefs::TouchVsyncSampleAdjust());
  mMaxPredict = TimeDuration::FromMilliseconds(gfxPrefs::TouchResampleMaxPredict());
  mOldTouchThreshold = TimeDuration::FromMilliseconds(gfxPrefs::TouchResampleOldTouchThreshold());
  mDelayedVsyncThreshold = TimeDuration::FromMilliseconds(gfxPrefs::TouchResampleVsyncDelayThreshold());
}

void
GeckoTouchDispatcher::SetCompositorVsyncScheduler(mozilla::layers::CompositorVsyncScheduler *aObserver)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  MOZ_ASSERT(mCompositorVsyncScheduler == nullptr);
  if (mResamplingEnabled) {
    mCompositorVsyncScheduler = aObserver;
  }
}

void
GeckoTouchDispatcher::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  MOZ_ASSERT(mResamplingEnabled);
  layers::APZThreadUtils::AssertOnControllerThread();
  DispatchTouchMoveEvents(aVsyncTimestamp);
}


void
GeckoTouchDispatcher::NotifyTouch(MultiTouchInput& aTouch, TimeStamp aEventTime)
{
  if (mCompositorVsyncScheduler) {
    mCompositorVsyncScheduler->SetNeedsComposite(true);
  }

  if (aTouch.mType == MultiTouchInput::MULTITOUCH_MOVE) {
    MutexAutoLock lock(mTouchQueueLock);
    if (mInflightNonMoveEvents > 0) {
      
      
      
      
      layers::APZThreadUtils::RunOnControllerThread(NewRunnableMethod(
        this, &GeckoTouchDispatcher::DispatchTouchEvent, aTouch));
      return;
    }

    mTouchMoveEvents.push_back(aTouch);
    mHavePendingTouchMoves = true;
    if (mResamplingEnabled) {
      return;
    }

    layers::APZThreadUtils::RunOnControllerThread(NewRunnableMethod(
      this, &GeckoTouchDispatcher::DispatchTouchMoveEvents, TimeStamp::Now()));
  } else {
    { 
      MutexAutoLock lock(mTouchQueueLock);
      mInflightNonMoveEvents++;
    }
    layers::APZThreadUtils::RunOnControllerThread(NewRunnableMethod(
      this, &GeckoTouchDispatcher::DispatchTouchNonMoveEvent, aTouch));
  }
}

void
GeckoTouchDispatcher::DispatchTouchNonMoveEvent(MultiTouchInput aInput)
{
  layers::APZThreadUtils::AssertOnControllerThread();

  if (mResamplingEnabled) {
    
    
    
    NotifyVsync(TimeStamp::Now());
  }
  DispatchTouchEvent(aInput);

  { 
    MutexAutoLock lock(mTouchQueueLock);
    mInflightNonMoveEvents--;
    MOZ_ASSERT(mInflightNonMoveEvents >= 0);
  }
}

void
GeckoTouchDispatcher::DispatchTouchMoveEvents(TimeStamp aVsyncTime)
{
  MultiTouchInput touchMove;

  {
    MutexAutoLock lock(mTouchQueueLock);
    if (!mHavePendingTouchMoves) {
      return;
    }
    mHavePendingTouchMoves = false;

    if (mResamplingEnabled) {
      int touchCount = mTouchMoveEvents.size();
      TimeDuration vsyncTouchDiff = aVsyncTime - mTouchMoveEvents.back().mTimeStamp;
      
      
      bool isDelayedVsyncEvent = vsyncTouchDiff < -mDelayedVsyncThreshold;
      bool isOldTouch = vsyncTouchDiff > mOldTouchThreshold;
      bool resample = (touchCount > 1) && !isDelayedVsyncEvent && !isOldTouch;

      if (!resample) {
        touchMove = mTouchMoveEvents.back();
        mTouchMoveEvents.clear();
        if (!isDelayedVsyncEvent && !isOldTouch) {
          mTouchMoveEvents.push_back(touchMove);
        }
      } else {
        ResampleTouchMoves(touchMove, aVsyncTime);
      }
    } else {
      touchMove = mTouchMoveEvents.back();
      mTouchMoveEvents.clear();
    }
  }

  DispatchTouchEvent(touchMove);
}

static int
Interpolate(int start, int end, TimeDuration aFrameDiff, TimeDuration aTouchDiff)
{
  return start + (((end - start) * aFrameDiff.ToMicroseconds()) / aTouchDiff.ToMicroseconds());
}

static const SingleTouchData&
GetTouchByID(const SingleTouchData& aCurrentTouch, MultiTouchInput& aOtherTouch)
{
  int32_t index = aOtherTouch.IndexOfTouch(aCurrentTouch.mIdentifier);
  if (index < 0) {
    
    
    
    
    return aCurrentTouch;
  }
  return aOtherTouch.mTouches[index];
}




static void
ResampleTouch(MultiTouchInput& aOutTouch,
              MultiTouchInput& aBase, MultiTouchInput& aCurrent,
              TimeDuration aFrameDiff, TimeDuration aTouchDiff)
{
  aOutTouch = aCurrent;

  
  for (size_t i = 0; i < aOutTouch.mTouches.Length(); i++) {
    const SingleTouchData& current = aCurrent.mTouches[i];
    const SingleTouchData& base = GetTouchByID(current, aBase);

    const ScreenIntPoint& baseTouchPoint = base.mScreenPoint;
    const ScreenIntPoint& currentTouchPoint = current.mScreenPoint;

    ScreenIntPoint newSamplePoint;
    newSamplePoint.x = Interpolate(baseTouchPoint.x, currentTouchPoint.x, aFrameDiff, aTouchDiff);
    newSamplePoint.y = Interpolate(baseTouchPoint.y, currentTouchPoint.y, aFrameDiff, aTouchDiff);

    aOutTouch.mTouches[i].mScreenPoint = newSamplePoint;

#ifdef LOG_RESAMPLE_DATA
    const char* type = "extrapolate";
    if (aFrameDiff < aTouchDiff) {
      type = "interpolate";
    }

    float alpha = aFrameDiff / aTouchDiff;
    LOG("%s base (%d, %d), current (%d, %d) to (%d, %d) alpha %f, touch diff %d, frame diff %d\n",
        type,
        baseTouchPoint.x, baseTouchPoint.y,
        currentTouchPoint.x, currentTouchPoint.y,
        newSamplePoint.x, newSamplePoint.y,
        alpha, (int)aTouchDiff.ToMilliseconds(), (int)aFrameDiff.ToMilliseconds());
#endif
  }
}


















void
GeckoTouchDispatcher::ResampleTouchMoves(MultiTouchInput& aOutTouch, TimeStamp aVsyncTime)
{
  MOZ_RELEASE_ASSERT(mTouchMoveEvents.size() >= 2);
  mTouchQueueLock.AssertCurrentThreadOwns();

  MultiTouchInput currentTouch = mTouchMoveEvents.back();
  mTouchMoveEvents.pop_back();
  MultiTouchInput baseTouch = mTouchMoveEvents.back();
  mTouchMoveEvents.clear();
  mTouchMoveEvents.push_back(currentTouch);

  TimeStamp sampleTime = aVsyncTime - mVsyncAdjust;
  TimeDuration touchDiff = currentTouch.mTimeStamp - baseTouch.mTimeStamp;

  if (currentTouch.mTimeStamp < sampleTime) {
    TimeDuration maxResampleTime = std::min(touchDiff / int64_t(2), mMaxPredict);
    TimeStamp maxTimestamp = currentTouch.mTimeStamp + maxResampleTime;
    if (sampleTime > maxTimestamp) {
      sampleTime = maxTimestamp;
      #ifdef LOG_RESAMPLE_DATA
      LOG("Overshot extrapolation time, adjusting sample time\n");
      #endif
    }
  }

  ResampleTouch(aOutTouch, baseTouch, currentTouch, sampleTime - baseTouch.mTimeStamp, touchDiff);

  
  
  
  aOutTouch.mTime += (sampleTime - aOutTouch.mTimeStamp).ToMilliseconds();
  aOutTouch.mTimeStamp = sampleTime;
}

static bool
IsExpired(const MultiTouchInput& aTouch)
{
  
  uint64_t timeNowMs = systemTime(SYSTEM_TIME_MONOTONIC) / 1000000;
  return (timeNowMs - aTouch.mTime) > kInputExpirationThresholdMs;
}
void
GeckoTouchDispatcher::DispatchTouchEvent(MultiTouchInput aMultiTouch)
{
  if ((aMultiTouch.mType == MultiTouchInput::MULTITOUCH_END ||
       aMultiTouch.mType == MultiTouchInput::MULTITOUCH_CANCEL) &&
      aMultiTouch.mTouches.Length() == 1) {
    MutexAutoLock lock(mTouchQueueLock);
    mTouchMoveEvents.clear();
  } else if (aMultiTouch.mType == MultiTouchInput::MULTITOUCH_START &&
             aMultiTouch.mTouches.Length() == 1) {
    mTouchEventsFiltered = IsExpired(aMultiTouch);
  }

  if (mTouchEventsFiltered) {
    return;
  }

  nsWindow::DispatchTouchInput(aMultiTouch);

  if (mEnabledUniformityInfo && profiler_is_active()) {
    const char* touchAction = "Invalid";
    switch (aMultiTouch.mType) {
      case MultiTouchInput::MULTITOUCH_START:
        touchAction = "Touch_Event_Down";
        break;
      case MultiTouchInput::MULTITOUCH_MOVE:
        touchAction = "Touch_Event_Move";
        break;
      case MultiTouchInput::MULTITOUCH_END:
      case MultiTouchInput::MULTITOUCH_CANCEL:
        touchAction = "Touch_Event_Up";
        break;
    }

    const ScreenIntPoint& touchPoint = aMultiTouch.mTouches[0].mScreenPoint;
    TouchDataPayload* payload = new TouchDataPayload(touchPoint);
    PROFILER_MARKER_PAYLOAD(touchAction, payload);
  }
}

} 
