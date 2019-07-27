
















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

GeckoTouchDispatcher::GeckoTouchDispatcher()
  : mTouchQueueLock("GeckoTouchDispatcher::mTouchQueueLock")
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
  sTouchDispatcher = this;
  ClearOnShutdown(&sTouchDispatcher);
}

class DispatchTouchEventsMainThread : public nsRunnable
{
public:
  DispatchTouchEventsMainThread(GeckoTouchDispatcher* aTouchDispatcher,
                                TimeStamp aVsyncTime)
    : mTouchDispatcher(aTouchDispatcher)
    , mVsyncTime(aVsyncTime)
  {
  }

  NS_IMETHOD Run()
  {
    mTouchDispatcher->DispatchTouchMoveEvents(mVsyncTime);
    return NS_OK;
  }

private:
  nsRefPtr<GeckoTouchDispatcher> mTouchDispatcher;
  TimeStamp mVsyncTime;
};

class DispatchSingleTouchMainThread : public nsRunnable
{
public:
  DispatchSingleTouchMainThread(GeckoTouchDispatcher* aTouchDispatcher,
                                MultiTouchInput& aTouch)
    : mTouchDispatcher(aTouchDispatcher)
    , mTouch(aTouch)
  {
  }

  NS_IMETHOD Run()
  {
    mTouchDispatcher->DispatchTouchEvent(mTouch);
    return NS_OK;
  }

private:
  nsRefPtr<GeckoTouchDispatcher> mTouchDispatcher;
  MultiTouchInput mTouch;
};

 void
GeckoTouchDispatcher::SetCompositorVsyncObserver(mozilla::layers::CompositorVsyncObserver *aObserver)
{
  MOZ_ASSERT(sTouchDispatcher != nullptr);
  MOZ_ASSERT(NS_IsMainThread());
  
  MOZ_ASSERT(sTouchDispatcher->mCompositorVsyncObserver == nullptr);
  if (gfxPrefs::TouchResampling()) {
    sTouchDispatcher->mCompositorVsyncObserver = aObserver;
  }
}


 bool
GeckoTouchDispatcher::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  if ((sTouchDispatcher == nullptr) || !gfxPrefs::TouchResampling()) {
    return false;
  }

  MOZ_ASSERT(sTouchDispatcher->mResamplingEnabled);
  bool haveTouchData = false;
  {
    MutexAutoLock lock(sTouchDispatcher->mTouchQueueLock);
    haveTouchData = !sTouchDispatcher->mTouchMoveEvents.empty();
  }

  if (haveTouchData) {
    NS_DispatchToMainThread(new DispatchTouchEventsMainThread(sTouchDispatcher, aVsyncTimestamp));
  }

  return haveTouchData;
}


void
GeckoTouchDispatcher::NotifyTouch(MultiTouchInput& aTouch, TimeStamp aEventTime)
{
  if (aTouch.mType == MultiTouchInput::MULTITOUCH_START && mCompositorVsyncObserver) {
    mCompositorVsyncObserver->SetNeedsComposite(true);
  }

  if (aTouch.mType == MultiTouchInput::MULTITOUCH_MOVE) {
    MutexAutoLock lock(mTouchQueueLock);
    if (mResamplingEnabled) {
      mTouchMoveEvents.push_back(aTouch);
      return;
    }

    if (mTouchMoveEvents.empty()) {
      mTouchMoveEvents.push_back(aTouch);
    } else {
      
      mTouchMoveEvents.back() = aTouch;
    }

    NS_DispatchToMainThread(new DispatchTouchEventsMainThread(this, TimeStamp::Now()));
  } else {
    NS_DispatchToMainThread(new DispatchSingleTouchMainThread(this, aTouch));
  }
}

void
GeckoTouchDispatcher::DispatchTouchMoveEvents(TimeStamp aVsyncTime)
{
  MultiTouchInput touchMove;

  {
    MutexAutoLock lock(mTouchQueueLock);
    if (mTouchMoveEvents.empty()) {
      return;
    }

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
    TimeDuration maxResampleTime = std::min(touchDiff / 2, mMaxPredict);
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
GeckoTouchDispatcher::DispatchTouchEvent(MultiTouchInput& aMultiTouch)
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
