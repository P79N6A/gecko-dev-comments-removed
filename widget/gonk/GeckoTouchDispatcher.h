
















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
  void NotifyTouch(MultiTouchInput& aData, uint64_t aEventTime);
  void DispatchTouchEvent(MultiTouchInput& aMultiTouch);
  void DispatchTouchMoveEvents(uint64_t aVsyncTime);
  static bool NotifyVsync(uint64_t aVsyncTimestamp);

private:
  int32_t InterpolateTouch(MultiTouchInput& aOutTouch, uint64_t aSampleTime);
  int32_t ExtrapolateTouch(MultiTouchInput& aOutTouch, uint64_t aSampleTime);
  void ResampleTouchMoves(MultiTouchInput& aOutTouch, uint64_t vsyncTime);
  void SendTouchEvent(MultiTouchInput& aData);
  void DispatchMouseEvent(MultiTouchInput& aMultiTouch,
                          bool aForwardToChildren);
  WidgetMouseEvent ToWidgetMouseEvent(const MultiTouchInput& aData, nsIWidget* aWidget) const;

  
  
  Mutex mTouchQueueLock;
  std::vector<MultiTouchInput> mTouchMoveEvents;

  bool mResamplingEnabled;
  bool mTouchEventsFiltered;
  bool mEnabledUniformityInfo;
  int mTouchDownCount;

  
  int32_t mVsyncAdjust;     
  int32_t mMaxPredict;      

  
  
  int32_t mMinResampleTime;

  
  int64_t mTouchTimeDiff;

  
  uint64_t mLastTouchTime;
};

} 
#endif 
