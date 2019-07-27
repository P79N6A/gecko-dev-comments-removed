
















#ifndef GECKO_TOUCH_INPUT_DISPATCHER_h
#define GECKO_TOUCH_INPUT_DISPATCHER_h

#include "InputData.h"
#include "Units.h"
#include "mozilla/Mutex.h"
#include <vector>
#include "nsRefPtr.h"

class nsIWidget;

namespace mozilla {
class WidgetMouseEvent;

namespace layers {
class CompositorVsyncObserver;
}











class GeckoTouchDispatcher
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GeckoTouchDispatcher)

public:
  GeckoTouchDispatcher();
  void NotifyTouch(MultiTouchInput& aTouch, TimeStamp aEventTime);
  void DispatchTouchEvent(MultiTouchInput& aMultiTouch);
  void DispatchTouchMoveEvents(TimeStamp aVsyncTime);
  static bool NotifyVsync(TimeStamp aVsyncTimestamp);
  static void SetCompositorVsyncObserver(layers::CompositorVsyncObserver* aObserver);

private:
  void ResampleTouchMoves(MultiTouchInput& aOutTouch, TimeStamp vsyncTime);
  void SendTouchEvent(MultiTouchInput& aData);
  void DispatchMouseEvent(MultiTouchInput& aMultiTouch,
                          bool aForwardToChildren);

  
  
  Mutex mTouchQueueLock;
  std::vector<MultiTouchInput> mTouchMoveEvents;

  bool mResamplingEnabled;
  bool mTouchEventsFiltered;
  bool mEnabledUniformityInfo;

  
  TimeDuration mVsyncAdjust;     
  TimeDuration mMaxPredict;      

  
  
  TimeDuration mMinResampleTime;

  
  TimeDuration mDelayedVsyncThreshold;

  
  TimeDuration mOldTouchThreshold;

  nsRefPtr<layers::CompositorVsyncObserver> mCompositorVsyncObserver;
};

} 
#endif 
