





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
  using RequestSampleCallback::NotDecodedReason;

  MediaDataDecodedListener(Target* aTarget,
                           MediaTaskQueue* aTaskQueue)
    : mMonitor("MediaDataDecodedListener")
    , mTaskQueue(aTaskQueue)
    , mTarget(aTarget)
  {
    MOZ_ASSERT(aTarget);
    MOZ_ASSERT(aTaskQueue);
  }

  virtual void OnAudioDecoded(AudioData* aSample) MOZ_OVERRIDE {
    MonitorAutoLock lock(mMonitor);
    if (!mTarget || !mTaskQueue) {
      
      return;
    }
    RefPtr<nsIRunnable> task(new DeliverAudioTask(aSample, mTarget));
    mTaskQueue->Dispatch(task);
  }

  virtual void OnVideoDecoded(VideoData* aSample) MOZ_OVERRIDE {
    MonitorAutoLock lock(mMonitor);
    if (!mTarget || !mTaskQueue) {
      
      return;
    }
    RefPtr<nsIRunnable> task(new DeliverVideoTask(aSample, mTarget));
    mTaskQueue->Dispatch(task);
  }

  virtual void OnNotDecoded(MediaData::Type aType, NotDecodedReason aReason) MOZ_OVERRIDE {
    MonitorAutoLock lock(mMonitor);
    if (!mTarget || !mTaskQueue) {
      
      return;
    }
    RefPtr<nsIRunnable> task(new DeliverNotDecodedTask(aType, aReason, mTarget));
    mTaskQueue->Dispatch(task);
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

  class DeliverAudioTask : public nsRunnable {
  public:
    DeliverAudioTask(AudioData* aSample, Target* aTarget)
      : mSample(aSample)
      , mTarget(aTarget)
    {
      MOZ_COUNT_CTOR(DeliverAudioTask);
    }
  protected:
    ~DeliverAudioTask()
    {
      MOZ_COUNT_DTOR(DeliverAudioTask);
    }
  public:
    NS_METHOD Run() {
      mTarget->OnAudioDecoded(mSample);
      return NS_OK;
    }
  private:
    nsRefPtr<AudioData> mSample;
    RefPtr<Target> mTarget;
  };

  class DeliverVideoTask : public nsRunnable {
  public:
    DeliverVideoTask(VideoData* aSample, Target* aTarget)
      : mSample(aSample)
      , mTarget(aTarget)
    {
      MOZ_COUNT_CTOR(DeliverVideoTask);
    }
  protected:
    ~DeliverVideoTask()
    {
      MOZ_COUNT_DTOR(DeliverVideoTask);
    }
  public:
    NS_METHOD Run() {
      mTarget->OnVideoDecoded(mSample);
      return NS_OK;
    }
  private:
    nsRefPtr<VideoData> mSample;
    RefPtr<Target> mTarget;
  };

  class DeliverNotDecodedTask : public nsRunnable {
  public:
    DeliverNotDecodedTask(MediaData::Type aType,
                          RequestSampleCallback::NotDecodedReason aReason,
                          Target* aTarget)
      : mType(aType)
      , mReason(aReason)
      , mTarget(aTarget)
    {
      MOZ_COUNT_CTOR(DeliverNotDecodedTask);
    }
  protected:
    ~DeliverNotDecodedTask()
    {
      MOZ_COUNT_DTOR(DeliverNotDecodedTask);
    }
  public:
    NS_METHOD Run() {
      mTarget->OnNotDecoded(mType, mReason);
      return NS_OK;
    }
  private:
    MediaData::Type mType;
    RequestSampleCallback::NotDecodedReason mReason;
    RefPtr<Target> mTarget;
  };

  Monitor mMonitor;
  RefPtr<MediaTaskQueue> mTaskQueue;
  RefPtr<Target> mTarget;
};

} 

#endif 
