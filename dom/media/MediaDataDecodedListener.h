





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

private:
  Monitor mMonitor;
  RefPtr<MediaTaskQueue> mTaskQueue;
  RefPtr<Target> mTarget;
};

} 

#endif 
