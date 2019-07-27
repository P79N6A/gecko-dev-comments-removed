




#include "mp4_demuxer/mp4_demuxer.h"
#include "GonkMediaDataDecoder.h"
#include "VideoUtils.h"
#include "nsTArray.h"
#include "MediaCodecProxy.h"

#include "prlog.h"
#define LOG_TAG "GonkMediaDataDecoder(blake)"
#include <android/log.h>
#define ALOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#ifdef PR_LOGGING
PRLogModuleInfo* GetDemuxerLog();
#define LOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

using namespace android;

namespace mozilla {

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
    ALOG("Failed to input data err: %d",rv);
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
  nsAutoPtr<MediaData> output;
  nsresult rv;
  while (true && !mDrainComplete) {
    rv = mManager->Output(mLastStreamOffset, output);
    if (rv == NS_OK) {
      mCallback->Output(output.forget());
      continue;
    } else if (rv == NS_ERROR_NOT_AVAILABLE && mSignaledEOS) {
      
      continue;
    }
    else {
      break;
    }
  }

  if (rv == NS_ERROR_NOT_AVAILABLE) {
    mCallback->InputExhausted();
    return;
  }
  if (rv != NS_OK) {
    NS_WARNING("GonkMediaDataDecoder failed to output data");
    ALOG("Failed to output data");
    
    if (rv == NS_ERROR_ABORT) {
      if (output.get() != nullptr) {
        mCallback->Output(output.forget());
      }
      mCallback->DrainComplete();
      mSignaledEOS = false;
      mDrainComplete = true;
      return;
    }
    mCallback->Error();
  }
}

nsresult
GonkMediaDataDecoder::Flush()
{
  
  
  
  
  mTaskQueue->Flush();

  status_t err = mDecoder->flush();
  return err == OK ? NS_OK : NS_ERROR_FAILURE;
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

bool
GonkMediaDataDecoder::IsDormantNeeded() {
  return mDecoder->IsDormantNeeded();
}

void
GonkMediaDataDecoder::ReleaseMediaResources() {
  mDecoder->ReleaseMediaResources();
}

} 
