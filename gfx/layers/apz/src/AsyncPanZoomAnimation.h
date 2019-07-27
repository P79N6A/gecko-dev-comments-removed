





#ifndef mozilla_layers_AsyncPanZoomAnimation_h_
#define mozilla_layers_AsyncPanZoomAnimation_h_

#include "base/message_loop.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "FrameMetrics.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace layers {

class WheelScrollAnimation;

class AsyncPanZoomAnimation {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncPanZoomAnimation)

public:
  explicit AsyncPanZoomAnimation(const TimeDuration& aRepaintInterval)
    : mRepaintInterval(aRepaintInterval)
  { }

  virtual bool DoSample(FrameMetrics& aFrameMetrics,
                        const TimeDuration& aDelta) = 0;

  bool Sample(FrameMetrics& aFrameMetrics,
              const TimeDuration& aDelta) {
    
    
    
    if (aDelta.ToMilliseconds() <= 0) {
      return true;
    }

    return DoSample(aFrameMetrics, aDelta);
  }

  




  Vector<Task*> TakeDeferredTasks() {
    Vector<Task*> result;
    mDeferredTasks.swap(result);
    return result;
  }

  




  TimeDuration mRepaintInterval;

public:
  virtual WheelScrollAnimation* AsWheelScrollAnimation() {
    return nullptr;
  }

protected:
  
  virtual ~AsyncPanZoomAnimation()
  { }

  




  Vector<Task*> mDeferredTasks;
};

} 
} 

#endif 
