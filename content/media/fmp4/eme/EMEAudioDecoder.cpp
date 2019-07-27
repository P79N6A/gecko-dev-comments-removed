





#include "EMEAudioDecoder.h"
#include "mp4_demuxer/DecoderData.h"
#include "mozilla/EMELog.h"
#include "gmp-audio-host.h"
#include "gmp-audio-decode.h"
#include "gmp-audio-samples.h"
#include "GMPAudioHost.h"
#include "GMPAudioDecoderProxy.h"
#include "mozilla/CDMProxy.h"
#include "nsServiceManagerUtils.h"
#include "prsystem.h"

namespace mozilla {

EMEAudioDecoder::EMEAudioDecoder(CDMProxy* aProxy,
                                 const AudioDecoderConfig& aConfig,
                                 MediaTaskQueue* aTaskQueue,
                                 MediaDataDecoderCallback* aCallback)
  : mAudioRate(0)
  , mAudioBytesPerSample(0)
  , mAudioChannels(0)
  , mMustRecaptureAudioPosition(true)
  , mAudioFrameSum(0)
  , mAudioFrameOffset(0)
  , mStreamOffset(0)
  , mProxy(aProxy)
  , mGMP(nullptr)
  , mConfig(aConfig)
  , mTaskQueue(aTaskQueue)
  , mCallback(aCallback)
  , mMonitor("EMEAudioDecoder")
  , mFlushComplete(false)
{
}

EMEAudioDecoder::~EMEAudioDecoder()
{
}

nsresult
EMEAudioDecoder::Init()
{
  

  MOZ_ASSERT((mConfig.bits_per_sample / 8) == 2); 

  mMPS = do_GetService("@mozilla.org/gecko-media-plugin-service;1");
  MOZ_ASSERT(mMPS);

  nsresult rv = mMPS->GetThread(getter_AddRefs(mGMPThread));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<InitTask> task(new InitTask(this));
  rv = mGMPThread->Dispatch(task, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(task->mResult, task->mResult);

  return NS_OK;
}

nsresult
EMEAudioDecoder::Input(MP4Sample* aSample)
{
  MOZ_ASSERT(!IsOnGMPThread()); 

  nsRefPtr<nsIRunnable> task(new DeliverSample(this, aSample));
  nsresult rv = mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
EMEAudioDecoder::Flush()
{
  MOZ_ASSERT(!IsOnGMPThread()); 

  {
    MonitorAutoLock mon(mMonitor);
    mFlushComplete = false;
  }

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(this, &EMEAudioDecoder::GmpFlush);
  nsresult rv = mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  {
    MonitorAutoLock mon(mMonitor);
    while (!mFlushComplete) {
      mon.Wait();
    }
  }

  return NS_OK;
}

nsresult
EMEAudioDecoder::Drain()
{
  MOZ_ASSERT(!IsOnGMPThread()); 

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(this, &EMEAudioDecoder::GmpDrain);
  nsresult rv = mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
EMEAudioDecoder::Shutdown()
{
  MOZ_ASSERT(!IsOnGMPThread()); 

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(this, &EMEAudioDecoder::GmpShutdown);
  nsresult rv = mGMPThread->Dispatch(task, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

void
EMEAudioDecoder::Decoded(const nsTArray<int16_t>& aPCM,
                         uint64_t aTimeStamp,
                         uint32_t aChannels,
                         uint32_t aRate)
{
  MOZ_ASSERT(IsOnGMPThread());

  if (aRate == 0 || aChannels == 0) {
    NS_WARNING("Invalid rate or num channels returned on GMP audio samples");
    mCallback->Error();
    return;
  }

  size_t numFrames = aPCM.Length() / aChannels;
  MOZ_ASSERT((aPCM.Length() % aChannels) == 0);
  nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[aPCM.Length()]);

  for (size_t i = 0; i < aPCM.Length(); ++i) {
    audioData[i] = AudioSampleToFloat(aPCM[i]);
  }

  if (mMustRecaptureAudioPosition) {
    mAudioFrameSum = 0;
    auto timestamp = UsecsToFrames(aTimeStamp, aRate);
    if (!timestamp.isValid()) {
      NS_WARNING("Invalid timestamp");
      mCallback->Error();
      return;
    }
    mAudioFrameOffset = timestamp.value();
    MOZ_ASSERT(mAudioFrameOffset >= 0);
    mMustRecaptureAudioPosition = false;
  }

  auto timestamp = FramesToUsecs(mAudioFrameOffset + mAudioFrameSum, aRate);
  if (!timestamp.isValid()) {
    NS_WARNING("Invalid timestamp on audio samples");
    mCallback->Error();
    return;
  }
  mAudioFrameSum += numFrames;

  auto duration = FramesToUsecs(numFrames, aRate);
  if (!duration.isValid()) {
    NS_WARNING("Invalid duration on audio samples");
    mCallback->Error();
    return;
  }

  nsAutoPtr<AudioData> audio(new AudioData(mStreamOffset,
                                           timestamp.value(),
                                           duration.value(),
                                           numFrames,
                                           audioData.forget(),
                                           aChannels,
                                           aRate));

  #ifdef LOG_SAMPLE_DECODE
  LOG("Decoded audio sample! timestamp=%lld duration=%lld currentLength=%u",
      timestamp, duration, currentLength);
  #endif

  mCallback->Output(audio.forget());
}

void
EMEAudioDecoder::InputDataExhausted()
{
  MOZ_ASSERT(IsOnGMPThread());
  mCallback->InputExhausted();
}

void
EMEAudioDecoder::DrainComplete()
{
  MOZ_ASSERT(IsOnGMPThread());
  mCallback->DrainComplete();
}

void
EMEAudioDecoder::ResetComplete()
{
  MOZ_ASSERT(IsOnGMPThread());
  mMustRecaptureAudioPosition = true;
  {
    MonitorAutoLock mon(mMonitor);
    mFlushComplete = true;
    mon.NotifyAll();
  }
}

void
EMEAudioDecoder::Error(GMPErr aErr)
{
  MOZ_ASSERT(IsOnGMPThread());
  EME_LOG("EMEAudioDecoder::Error");
  mCallback->Error();
  GmpShutdown();
}

void
EMEAudioDecoder::Terminated()
{
  MOZ_ASSERT(IsOnGMPThread());
  GmpShutdown();
}

nsresult
EMEAudioDecoder::GmpInit()
{
  MOZ_ASSERT(IsOnGMPThread());

  nsTArray<nsCString> tags;
  tags.AppendElement(NS_LITERAL_CSTRING("aac"));
  tags.AppendElement(NS_ConvertUTF16toUTF8(mProxy->KeySystem()));
  nsresult rv = mMPS->GetGMPAudioDecoder(&tags,
                                         mProxy->GetOrigin(),
                                         &mGMP);
  NS_ENSURE_SUCCESS(rv, rv);
  MOZ_ASSERT(mGMP);

  mAudioRate = mConfig.samples_per_second;
  mAudioBytesPerSample = mConfig.bits_per_sample / 8;
  mAudioChannels = mConfig.channel_count;

  nsTArray<uint8_t> extraData;
  extraData.AppendElements(&mConfig.audio_specific_config[0],
                           mConfig.audio_specific_config.length());

  mGMP->InitDecode(kGMPAudioCodecAAC,
                   mAudioChannels,
                   mConfig.bits_per_sample,
                   mAudioRate,
                   extraData,
                   this);

  return NS_OK;
}

nsresult
EMEAudioDecoder::GmpInput(MP4Sample* aSample)
{
  MOZ_ASSERT(IsOnGMPThread());
  nsAutoPtr<MP4Sample> sample(aSample);
  if (!mGMP) {
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  if (sample->crypto.valid) {
    CDMCaps::AutoLock caps(mProxy->Capabilites());
    MOZ_ASSERT(caps.CanDecryptAndDecodeAudio());
    const auto& keyid = sample->crypto.key;
    if (!caps.IsKeyUsable(keyid)) {
      
      nsRefPtr<nsIRunnable> task(new DeliverSample(this, sample.forget()));
      caps.CallWhenKeyUsable(keyid, task, mGMPThread);
      return NS_OK;
    }
  }

  gmp::GMPAudioSamplesImpl samples(sample, mAudioChannels, mAudioRate);
  mGMP->Decode(samples);

  mStreamOffset = sample->byte_offset;

  return NS_OK;
}

void
EMEAudioDecoder::GmpFlush()
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mGMP || NS_FAILED(mGMP->Reset())) {
    
    MonitorAutoLock mon(mMonitor);
    mFlushComplete = true;
    mon.NotifyAll();
  }
}

void
EMEAudioDecoder::GmpDrain()
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mGMP || NS_FAILED(mGMP->Drain())) {
    mCallback->DrainComplete();
  }
}

void
EMEAudioDecoder::GmpShutdown()
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mGMP) {
    return;
  }
  mGMP->Close();
  mGMP = nullptr;
}

} 
