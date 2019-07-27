
















#ifndef GECKO_TOUCH_INPUT_DISPATCHER_h
#define GECKO_TOUCH_INPUT_DISPATCHER_h

#include "InputData.h"
#include "Units.h"
#include "mozilla/Mutex.h"
#include <vector>
#include "nsRefPtr.h"

class nsIWidget;

namespace mozilla {
namespace layers {
class CompositorVsyncScheduler;
}











class GeckoTouchDispatcher final
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GeckoTouchDispatcher)

public:
  static GeckoTouchDispatcher* GetInstance();
  void NotifyTouch(MultiTouchInput& aTouch, TimeStamp aEventTime);
  void DispatchTouchEvent(MultiTouchInput aMultiTouch);
  void DispatchTouchNonMoveEvent(MultiTouchInput aInput);
  void DispatchTouchMoveEvents(TimeStamp aVsyncTime);
  void NotifyVsync(TimeStamp aVsyncTimestamp);
  void SetCompositorVsyncScheduler(layers::CompositorVsyncScheduler* aObserver);

protected:
  ~GeckoTouchDispatcher() {}

private:
  GeckoTouchDispatcher();
  void ResampleTouchMoves(MultiTouchInput& aOutTouch, TimeStamp vsyncTime);
  void SendTouchEvent(MultiTouchInput& aData);
  void DispatchMouseEvent(MultiTouchInput& aMultiTouch,
                          bool aForwardToChildren);

  
  
  Mutex mTouchQueueLock;
  std::vector<MultiTouchInput> mTouchMoveEvents;
  bool mHavePendingTouchMoves;
  int mInflightNonMoveEvents;
  

  bool mResamplingEnabled;
  bool mTouchEventsFiltered;
  bool mEnabledUniformityInfo;

  
  TimeDuration mVsyncAdjust;     
  TimeDuration mMaxPredict;      

  
  
  TimeDuration mMinResampleTime;

  
  TimeDuration mDelayedVsyncThreshold;

  
  TimeDuration mOldTouchThreshold;

  nsRefPtr<layers::CompositorVsyncScheduler> mCompositorVsyncScheduler;
};

} 
#endif 
