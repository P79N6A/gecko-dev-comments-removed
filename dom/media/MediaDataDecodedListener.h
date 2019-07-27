





#ifndef MediaDataDecodedListener_h_
#define MediaDataDecodedListener_h_

#include "mozilla/Monitor.h"
#include "MediaDecoderReader.h"

namespace mozilla {

class MediaDecoderStateMachine;
class MediaData;



template<class Target>
class MediaDataDecodedListener : public RequestSampleCallback {
public:
  MediaDataDecodedListener(Target* aTarget,
                           MediaTaskQueue* aTaskQueue)
    : mMonitor("MediaDataDecodedListener")
    , mTaskQueue(aTaskQueue)
    , mTarget(aTarget)
  {
    MOZ_ASSERT(aTarget);
    MOZ_ASSERT(aTaskQueue);
  }

  void BreakCycles() {
    MonitorAutoLock lock(mMonitor);
    mTarget = nullptr;
    mTaskQueue = nullptr;
  }

  virtual void OnSeekCompleted(nsresult aResult) MOZ_OVERRIDE {
    MonitorAutoLock lock(mMonitor);
    if (!mTarget || !mTaskQueue) {
      
      return;
    }
    RefPtr<nsIRunnable> task(NS_NewRunnableMethodWithArg<nsresult>(mTarget,
                                                                   &Target::OnSeekCompleted,
                                                                   aResult));
    if (NS_FAILED(mTaskQueue->Dispatch(task))) {
      NS_WARNING("Failed to dispatch OnSeekCompleted task");
    }
  }

private:
  Monitor mMonitor;
  RefPtr<MediaTaskQueue> mTaskQueue;
  RefPtr<Target> mTarget;
};

} 

#endif 
