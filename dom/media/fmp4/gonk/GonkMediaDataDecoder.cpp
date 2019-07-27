




#include "mp4_demuxer/mp4_demuxer.h"
#include "GonkMediaDataDecoder.h"
#include "VideoUtils.h"
#include "nsTArray.h"
#include "MediaCodecProxy.h"

#include "prlog.h"
#include <android/log.h>
#define GMDD_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, "GonkMediaDataDecoder", __VA_ARGS__)

#ifdef PR_LOGGING
PRLogModuleInfo* GetDemuxerLog();
#define LOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

using namespace android;

namespace mozilla {

GonkDecoderManager::GonkDecoderManager(MediaTaskQueue* aTaskQueue)
  : mTaskQueue(aTaskQueue)
{
}

nsresult
GonkDecoderManager::Input(mp4_demuxer::MP4Sample* aSample)
{
  MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());

  
  
  
  
  uint32_t len = mQueueSample.Length();
  status_t rv = OK;

  for (uint32_t i = 0; i < len; i++) {
    rv = SendSampleToOMX(mQueueSample.ElementAt(0));
    if (rv != OK) {
      break;
    }
    mQueueSample.RemoveElementAt(0);
  }

  
  
  nsAutoPtr<mp4_demuxer::MP4Sample> sample;
  if (!aSample) {
    sample = new mp4_demuxer::MP4Sample();
  }

  
  
  if (rv == OK) {
    MOZ_ASSERT(!mQueueSample.Length());
    mp4_demuxer::MP4Sample* tmp;
    if (aSample) {
      tmp = aSample;
      PerformFormatSpecificProcess(aSample);
    } else {
      tmp = sample;
    }
    rv = SendSampleToOMX(tmp);
    if (rv == OK) {
      return NS_OK;
    }
  }

  
  
  if (!sample) {
      sample = aSample->Clone();
      if (!sample) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
  }
  mQueueSample.AppendElement(sample);

  
  
  if (rv == -EAGAIN || rv == -ETIMEDOUT) {
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
}

nsresult
GonkDecoderManager::Flush()
{
  class ClearQueueRunnable : public nsRunnable
  {
  public:
    explicit ClearQueueRunnable(GonkDecoderManager* aManager)
      : mManager(aManager) {}

    NS_IMETHOD Run()
    {
      mManager->ClearQueuedSample();
      return NS_OK;
    }

    GonkDecoderManager* mManager;
  };

  mTaskQueue->SyncDispatch(new ClearQueueRunnable(this));
  return NS_OK;
}

GonkMediaDataDecoder::GonkMediaDataDecoder(GonkDecoderManager* aManager,
                                           MediaTaskQueue* aTaskQueue,
                                           MediaDataDecoderCallback* aCallback)
  : mTaskQueue(aTaskQueue)
  , mCallback(aCallback)
  , mManager(aManager)
  , mSignaledEOS(false)
  , mDrainComplete(false)
{
  MOZ_COUNT_CTOR(GonkMediaDataDecoder);
}

GonkMediaDataDecoder::~GonkMediaDataDecoder()
{
  MOZ_COUNT_DTOR(GonkMediaDataDecoder);
}

nsresult
GonkMediaDataDecoder::Init()
{
  mDecoder = mManager->Init(mCallback);
  mDrainComplete = false;
  return mDecoder.get() ? NS_OK : NS_ERROR_UNEXPECTED;
}

nsresult
GonkMediaDataDecoder::Shutdown()
{
  mDecoder->stop();
  mDecoder = nullptr;
  return NS_OK;
}


nsresult
GonkMediaDataDecoder::Input(mp4_demuxer::MP4Sample* aSample)
{
  mTaskQueue->Dispatch(
    NS_NewRunnableMethodWithArg<nsAutoPtr<mp4_demuxer::MP4Sample>>(
      this,
      &GonkMediaDataDecoder::ProcessDecode,
      nsAutoPtr<mp4_demuxer::MP4Sample>(aSample)));
  return NS_OK;
}

void
GonkMediaDataDecoder::ProcessDecode(mp4_demuxer::MP4Sample* aSample)
{
  nsresult rv = mManager->Input(aSample);
  if (rv != NS_OK) {
    NS_WARNING("GonkAudioDecoder failed to input data");
    GMDD_LOG("Failed to input data err: %d",rv);
    mCallback->Error();
    return;
  }
  if (aSample) {
    mLastStreamOffset = aSample->byte_offset;
  }
  ProcessOutput();
}

void
GonkMediaDataDecoder::ProcessOutput()
{
  nsRefPtr<MediaData> output;
  nsresult rv = NS_ERROR_ABORT;

  while (!mDrainComplete) {
    
    if (mSignaledEOS && mManager->HasQueuedSample()) {
      GMDD_LOG("ProcessOutput: drain all input samples");
      rv = mManager->Input(nullptr);
    }
    rv = mManager->Output(mLastStreamOffset, output);
    if (rv == NS_OK) {
      mCallback->Output(output);
      continue;
    } else if (rv == NS_ERROR_NOT_AVAILABLE && mSignaledEOS) {
      
      continue;
    }
    else {
      break;
    }
  }

  MOZ_ASSERT_IF(mSignaledEOS, !mManager->HasQueuedSample());

  if (rv == NS_ERROR_NOT_AVAILABLE && !mSignaledEOS) {
    mCallback->InputExhausted();
    return;
  }
  if (rv != NS_OK) {
    NS_WARNING("GonkMediaDataDecoder failed to output data");
    GMDD_LOG("Failed to output data");
    
    if (rv == NS_ERROR_ABORT) {
      if (output) {
        mCallback->Output(output);
      }
      mCallback->DrainComplete();
      mSignaledEOS = false;
      mDrainComplete = true;
      return;
    }
    GMDD_LOG("Callback error!");
    mCallback->Error();
  }
}

nsresult
GonkMediaDataDecoder::Flush()
{
  
  
  
  
  mTaskQueue->Flush();

  return mManager->Flush();
}

void
GonkMediaDataDecoder::ProcessDrain()
{
  
  ProcessDecode(nullptr);
  mSignaledEOS = true;
  ProcessOutput();
}

nsresult
GonkMediaDataDecoder::Drain()
{
  mTaskQueue->Dispatch(NS_NewRunnableMethod(this, &GonkMediaDataDecoder::ProcessDrain));
  return NS_OK;
}

bool
GonkMediaDataDecoder::IsWaitingMediaResources() {
  return mDecoder->IsWaitingResources();
}

void
GonkMediaDataDecoder::AllocateMediaResources()
{
  mManager->AllocateMediaResources();
}

void
GonkMediaDataDecoder::ReleaseMediaResources()
{
  mManager->ReleaseMediaResources();
}

} 
