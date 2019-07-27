
















#ifndef GECKO_TOUCH_INPUT_DISPATCHER_h
#define GECKO_TOUCH_INPUT_DISPATCHER_h

#include "InputData.h"
#include "Units.h"
#include "mozilla/Mutex.h"
#include <vector>

class nsIWidget;

namespace mozilla {
class WidgetMouseEvent;











class GeckoTouchDispatcher
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GeckoTouchDispatcher)

public:
  GeckoTouchDispatcher();
  void NotifyTouch(MultiTouchInput& aTouch, TimeStamp aEventTime);
  void DispatchTouchEvent(MultiTouchInput& aMultiTouch);
  void DispatchTouchMoveEvents(TimeStamp aVsyncTime);
  static bool NotifyVsync(TimeStamp aVsyncTimestamp);

private:
  void ResampleTouchMoves(MultiTouchInput& aOutTouch, TimeStamp vsyncTime);
  void SendTouchEvent(MultiTouchInput& aData);
  void DispatchMouseEvent(MultiTouchInput& aMultiTouch,
                          bool aForwardToChildren);
  WidgetMouseEvent ToWidgetMouseEvent(const MultiTouchInput& aData, nsIWidget* aWidget) const;

  
  
  Mutex mTouchQueueLock;
  std::vector<MultiTouchInput> mTouchMoveEvents;

  bool mResamplingEnabled;
  bool mTouchEventsFiltered;
  bool mEnabledUniformityInfo;

  
  TimeDuration mVsyncAdjust;     
  TimeDuration mMaxPredict;      

  
  
  TimeDuration mMinResampleTime;

  
  TimeDuration mTouchTimeDiff;

  
  TimeStamp mLastTouchTime;

  
  TimeDuration mDelayedVsyncThreshold;
};

} 
#endif 
