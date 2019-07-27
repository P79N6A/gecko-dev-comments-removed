
















#include "FrameMetrics.h"
#include "GeckoProfiler.h"
#include "GeckoTouchDispatcher.h"
#include "InputData.h"
#include "base/basictypes.h"
#include "gfxPrefs.h"
#include "libui/Input.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/TouchEvents.h"
#include "mozilla/dom/Touch.h"
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
static int32_t nanosecToMillisec(int64_t nanosec) { return nanosec / 1000000; }

static StaticRefPtr<GeckoTouchDispatcher> sTouchDispatcher;

GeckoTouchDispatcher::GeckoTouchDispatcher()
  : mTouchQueueLock("GeckoTouchDispatcher::mTouchQueueLock")
  , mTouchEventsFiltered(false)
  , mTouchDownCount(0)
  , mTouchTimeDiff(0)
  , mLastTouchTime(0)
{
  
  
  
  
  MOZ_ASSERT(sTouchDispatcher == nullptr);
  MOZ_ASSERT(NS_IsMainThread());
  gfxPrefs::GetSingleton();

  mEnabledUniformityInfo = gfxPrefs::UniformityInfo();
  mResamplingEnabled = gfxPrefs::TouchResampling() &&
                       gfxPrefs::FrameUniformityHWVsyncEnabled();
  mVsyncAdjust = gfxPrefs::TouchVsyncSampleAdjust();
  mMaxPredict = gfxPrefs::TouchResampleMaxPredict();
  mMinResampleTime = gfxPrefs::TouchResampleMinTime();
  sTouchDispatcher = this;
}

class DispatchTouchEventsMainThread : public nsRunnable
{
public:
  DispatchTouchEventsMainThread(GeckoTouchDispatcher* aTouchDispatcher,
                                uint64_t aVsyncTime)
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
  uint64_t mVsyncTime;
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


 bool
GeckoTouchDispatcher::NotifyVsync(uint64_t aVsyncTimestamp)
{
  if (sTouchDispatcher == nullptr) {
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
GeckoTouchDispatcher::NotifyTouch(MultiTouchInput& aData, uint64_t aEventTime)
{
  if (mResamplingEnabled) {
    switch (aData.mType) {
      case MultiTouchInput::MULTITOUCH_MOVE:
      {
        MutexAutoLock lock(mTouchQueueLock);
        mTouchMoveEvents.push_back(aData);
        mTouchTimeDiff = aEventTime - mLastTouchTime;
        mLastTouchTime = aEventTime;
        return;
      }
      default:
        break;
    }
  }

  NS_DispatchToMainThread(new DispatchSingleTouchMainThread(this, aData));
}

void
GeckoTouchDispatcher::DispatchTouchMoveEvents(uint64_t aVsyncTime)
{
  MultiTouchInput touchMove;

  {
    MutexAutoLock lock(mTouchQueueLock);
    if (mTouchMoveEvents.empty()) {
      return;
    }

    int touchCount = mTouchMoveEvents.size();
    
    
    int64_t vsyncTouchDiff = aVsyncTime - mLastTouchTime;
    bool resample = (touchCount > 1) &&
                    (vsyncTouchDiff > mMinResampleTime);

    if (!resample) {
      touchMove = mTouchMoveEvents.back();
      mTouchMoveEvents.clear();
      mTouchMoveEvents.push_back(touchMove);
    } else {
      ResampleTouchMoves(touchMove, aVsyncTime);
    }
  }

  DispatchTouchEvent(touchMove);
}

static int
Interpolate(int start, int end, int64_t aFrameDiff, int64_t aTouchDiff)
{
  return start + (((end - start) * aFrameDiff) / aTouchDiff);
}

static const SingleTouchData&
GetTouchByID(const SingleTouchData& aCurrentTouch, MultiTouchInput& aOtherTouch)
{
  int32_t id = aCurrentTouch.mIdentifier;
  for (size_t i = 0; i < aOtherTouch.mTouches.Length(); i++) {
    SingleTouchData& touch = aOtherTouch.mTouches[i];
    if (touch.mIdentifier == id) {
      return touch;
    }
  }

  
  
  
  
  return aCurrentTouch;
}

static void
ResampleTouch(MultiTouchInput& aOutTouch, MultiTouchInput& aCurrent,
              MultiTouchInput& aOther, int64_t aFrameDiff,
              int64_t aTouchDiff, bool aInterpolate)
{
  aOutTouch = aCurrent;

  
  for (size_t i = 0; i < aOutTouch.mTouches.Length(); i++) {
    const SingleTouchData& current = aCurrent.mTouches[i];
    const SingleTouchData& other = GetTouchByID(current, aOther);

    const ScreenIntPoint& currentTouchPoint = current.mScreenPoint;
    const ScreenIntPoint& otherTouchPoint = other.mScreenPoint;

    ScreenIntPoint newSamplePoint;
    newSamplePoint.x = Interpolate(currentTouchPoint.x, otherTouchPoint.x, aFrameDiff, aTouchDiff);
    newSamplePoint.y = Interpolate(currentTouchPoint.y, otherTouchPoint.y, aFrameDiff, aTouchDiff);

    aOutTouch.mTouches[i].mScreenPoint = newSamplePoint;

#ifdef LOG_RESAMPLE_DATA
    const char* type = "extrapolate";
    if (aInterpolate) {
      type = "interpolate";
    }

    float alpha = (double) aFrameDiff / (double) aTouchDiff;
    LOG("%s current (%d, %d), other (%d, %d) to (%d, %d) alpha %f, touch diff %llu, frame diff %lld\n",
        type,
        currentTouchPoint.x, currentTouchPoint.y,
        otherTouchPoint.x, otherTouchPoint.y,
        newSamplePoint.x, newSamplePoint.y,
        alpha, aTouchDiff, aFrameDiff);
#endif
  }
}



int32_t
GeckoTouchDispatcher::InterpolateTouch(MultiTouchInput& aOutTouch, uint64_t aSampleTime)
{
  MOZ_RELEASE_ASSERT(mTouchMoveEvents.size() >= 2);
  mTouchQueueLock.AssertCurrentThreadOwns();

  
  MultiTouchInput futureTouch = mTouchMoveEvents.back();
  mTouchMoveEvents.pop_back();
  MultiTouchInput currentTouch = mTouchMoveEvents.back();

  mTouchMoveEvents.clear();
  mTouchMoveEvents.push_back(futureTouch);

  uint64_t currentTouchTime = mLastTouchTime - mTouchTimeDiff;
  int64_t frameDiff = aSampleTime - currentTouchTime;
  ResampleTouch(aOutTouch, currentTouch, futureTouch, frameDiff, mTouchTimeDiff, true);

  return nanosecToMillisec(frameDiff);
}



int32_t
GeckoTouchDispatcher::ExtrapolateTouch(MultiTouchInput& aOutTouch, uint64_t aSampleTime)
{
  MOZ_RELEASE_ASSERT(mTouchMoveEvents.size() >= 2);
  mTouchQueueLock.AssertCurrentThreadOwns();

  
  MultiTouchInput currentTouch = mTouchMoveEvents.back();
  mTouchMoveEvents.pop_back();
  MultiTouchInput prevTouch = mTouchMoveEvents.back();
  mTouchMoveEvents.clear();
  mTouchMoveEvents.push_back(currentTouch);

  uint64_t currentTouchTime = mLastTouchTime;
  int64_t maxResampleTime = std::min(mTouchTimeDiff / 2, (int64_t) mMaxPredict);
  uint64_t maxTimestamp = currentTouchTime + maxResampleTime;

  if (aSampleTime > maxTimestamp) {
    aSampleTime = maxTimestamp;
    #ifdef LOG_RESAMPLE_DATA
    LOG("Overshot extrapolation time, adjusting sample time\n");
    #endif
  }

  
  int64_t frameDiff = currentTouchTime - aSampleTime;
  ResampleTouch(aOutTouch, currentTouch, prevTouch, frameDiff, mTouchTimeDiff, false);
  return -nanosecToMillisec(frameDiff);
}

void
GeckoTouchDispatcher::ResampleTouchMoves(MultiTouchInput& aOutTouch, uint64_t aVsyncTime)
{
  uint64_t sampleTime = aVsyncTime - mVsyncAdjust;
  int32_t touchTimeAdjust = 0;

  if (mLastTouchTime > sampleTime) {
    touchTimeAdjust = InterpolateTouch(aOutTouch, sampleTime);
  } else {
    touchTimeAdjust = ExtrapolateTouch(aOutTouch, sampleTime);
  }

  aOutTouch.mTimeStamp += TimeDuration::FromMilliseconds(touchTimeAdjust);
  aOutTouch.mTime += touchTimeAdjust;
}



void
GeckoTouchDispatcher::DispatchMouseEvent(MultiTouchInput& aMultiTouch,
                                         bool aForwardToChildren)
{
  WidgetMouseEvent mouseEvent = ToWidgetMouseEvent(aMultiTouch, nullptr);
  if (mouseEvent.message == NS_EVENT_NULL) {
    return;
  }

  mouseEvent.mFlags.mNoCrossProcessBoundaryForwarding = !aForwardToChildren;
  nsWindow::DispatchInputEvent(mouseEvent);
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
  if (!mTouchDownCount) {
    mTouchEventsFiltered = IsExpired(aMultiTouch);
  }

  switch (aMultiTouch.mType) {
    case MultiTouchInput::MULTITOUCH_START:
      mTouchDownCount++;
      break;
    case MultiTouchInput::MULTITOUCH_MOVE:
      break;
    case MultiTouchInput::MULTITOUCH_END:
    case MultiTouchInput::MULTITOUCH_CANCEL:
      mTouchDownCount--;
      if (mTouchDownCount == 0) {
        MutexAutoLock lock(mTouchQueueLock);
        mTouchMoveEvents.clear();
      }
      break;
    default:
      break;
  }

  if (mTouchEventsFiltered) {
    return;
  }

  bool captured = false;
  WidgetTouchEvent event = aMultiTouch.ToWidgetTouchEvent(nullptr);
  nsEventStatus status = nsWindow::DispatchInputEvent(event, &captured);

  if (mEnabledUniformityInfo) {
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

    const SingleTouchData& firstTouch = aMultiTouch.mTouches[0];
    const ScreenIntPoint& touchPoint = firstTouch.mScreenPoint;

    LOG("UniformityInfo %s %llu %d %d", touchAction, systemTime(SYSTEM_TIME_MONOTONIC),
        touchPoint.x, touchPoint.y);
  }

  if (!captured && (aMultiTouch.mTouches.Length() == 1)) {
    bool forwardToChildren = status != nsEventStatus_eConsumeNoDefault;
    DispatchMouseEvent(aMultiTouch, forwardToChildren);
  }
}

WidgetMouseEvent
GeckoTouchDispatcher::ToWidgetMouseEvent(const MultiTouchInput& aMultiTouch,
                                         nsIWidget* aWidget) const
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(),
                    "Can only convert To WidgetMouseEvent on main thread");

  uint32_t mouseEventType = NS_EVENT_NULL;
  switch (aMultiTouch.mType) {
    case MultiTouchInput::MULTITOUCH_START:
      mouseEventType = NS_MOUSE_BUTTON_DOWN;
      break;
    case MultiTouchInput::MULTITOUCH_MOVE:
      mouseEventType = NS_MOUSE_MOVE;
      break;
    case MultiTouchInput::MULTITOUCH_CANCEL:
    case MultiTouchInput::MULTITOUCH_END:
      mouseEventType = NS_MOUSE_BUTTON_UP;
      break;
    default:
      MOZ_ASSERT_UNREACHABLE("Did not assign a type to WidgetMouseEvent");
      break;
  }

  WidgetMouseEvent event(true, mouseEventType, aWidget,
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);

  const SingleTouchData& firstTouch = aMultiTouch.mTouches[0];
  event.refPoint.x = firstTouch.mScreenPoint.x;
  event.refPoint.y = firstTouch.mScreenPoint.y;

  event.time = aMultiTouch.mTime;
  event.button = WidgetMouseEvent::eLeftButton;
  event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  event.modifiers = aMultiTouch.modifiers;

  if (mouseEventType != NS_MOUSE_MOVE) {
    event.clickCount = 1;
  }

  return event;
}

} 
