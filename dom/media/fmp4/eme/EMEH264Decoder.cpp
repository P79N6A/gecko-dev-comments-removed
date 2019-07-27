





#include "EMEH264Decoder.h"
#include "gmp-video-host.h"
#include "gmp-video-decode.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"
#include "GMPVideoEncodedFrameImpl.h"
#include "mp4_demuxer/AnnexB.h"
#include "mozilla/CDMProxy.h"
#include "nsServiceManagerUtils.h"
#include "prsystem.h"
#include "gfx2DGlue.h"
#include "mozilla/EMELog.h"
#include "mozilla/Move.h"

namespace mozilla {

EMEH264Decoder::EMEH264Decoder(CDMProxy* aProxy,
                               const mp4_demuxer::VideoDecoderConfig& aConfig,
                               layers::LayersBackend aLayersBackend,
                               layers::ImageContainer* aImageContainer,
                               MediaTaskQueue* aTaskQueue,
                               MediaDataDecoderCallback* aCallback)
  : mProxy(aProxy)
  , mGMP(nullptr)
  , mHost(nullptr)
  , mConfig(aConfig)
  , mImageContainer(aImageContainer)
  , mTaskQueue(aTaskQueue)
  , mCallback(aCallback)
  , mLastStreamOffset(0)
  , mSamplesWaitingForKey(new SamplesWaitingForKey(this, mTaskQueue, mProxy))
  , mMonitor("EMEH264Decoder")
  , mFlushComplete(false)
#ifdef DEBUG
  , mIsShutdown(false)
#endif
{
}

EMEH264Decoder::~EMEH264Decoder() {
}

nsresult
EMEH264Decoder::Init()
{
  
  MOZ_ASSERT(!mIsShutdown);

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
EMEH264Decoder::Input(MP4Sample* aSample)
{
  MOZ_ASSERT(!IsOnGMPThread()); 
  MOZ_ASSERT(!mIsShutdown);

  if (mSamplesWaitingForKey->WaitIfKeyNotUsable(aSample)) {
    return NS_OK;
  }

  nsRefPtr<nsIRunnable> task(new DeliverSample(this, aSample));
  nsresult rv = mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
EMEH264Decoder::Flush()
{
  MOZ_ASSERT(!IsOnGMPThread()); 
  MOZ_ASSERT(!mIsShutdown);

  {
    MonitorAutoLock mon(mMonitor);
    mFlushComplete = false;
  }

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(this, &EMEH264Decoder::GmpFlush);
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
EMEH264Decoder::Drain()
{
  MOZ_ASSERT(!IsOnGMPThread()); 
  MOZ_ASSERT(!mIsShutdown);

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(this, &EMEH264Decoder::GmpDrain);
  nsresult rv = mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
EMEH264Decoder::Shutdown()
{
  MOZ_ASSERT(!IsOnGMPThread()); 
  MOZ_ASSERT(!mIsShutdown);
#ifdef DEBUG
  mIsShutdown = true;
#endif

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(this, &EMEH264Decoder::GmpShutdown);
  nsresult rv = mGMPThread->Dispatch(task, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);

  mSamplesWaitingForKey->BreakCycles();
  mSamplesWaitingForKey = nullptr;

  return NS_OK;
}

void
EMEH264Decoder::Decoded(GMPVideoi420Frame* aDecodedFrame)
{
  MOZ_ASSERT(IsOnGMPThread());

  VideoData::YCbCrBuffer b;

  auto height = aDecodedFrame->Height();
  auto width = aDecodedFrame->Width();

  
  b.mPlanes[0].mData = aDecodedFrame->Buffer(kGMPYPlane);
  b.mPlanes[0].mStride = aDecodedFrame->Stride(kGMPYPlane);
  b.mPlanes[0].mHeight = height;
  b.mPlanes[0].mWidth = width;
  b.mPlanes[0].mOffset = 0;
  b.mPlanes[0].mSkip = 0;

  
  b.mPlanes[1].mData = aDecodedFrame->Buffer(kGMPUPlane);
  b.mPlanes[1].mStride = aDecodedFrame->Stride(kGMPUPlane);
  b.mPlanes[1].mHeight = height / 2;
  b.mPlanes[1].mWidth = width / 2;
  b.mPlanes[1].mOffset = 0;
  b.mPlanes[1].mSkip = 0;

  
  b.mPlanes[2].mData = aDecodedFrame->Buffer(kGMPVPlane);
  b.mPlanes[2].mStride = aDecodedFrame->Stride(kGMPVPlane);
  b.mPlanes[2].mHeight = height / 2;
  b.mPlanes[2].mWidth = width / 2;
  b.mPlanes[2].mOffset = 0;
  b.mPlanes[2].mSkip = 0;

  gfx::IntRect pictureRegion(0, 0, width, height);
  nsRefPtr<VideoData> v = VideoData::Create(mVideoInfo,
                                            mImageContainer,
                                            mLastStreamOffset,
                                            aDecodedFrame->Timestamp(),
                                            aDecodedFrame->Duration(),
                                            b,
                                            false,
                                            -1,
                                            pictureRegion);
  aDecodedFrame->Destroy();
  mCallback->Output(v);
}

void
EMEH264Decoder::ReceivedDecodedReferenceFrame(const uint64_t aPictureId)
{
  
}

void
EMEH264Decoder::ReceivedDecodedFrame(const uint64_t aPictureId)
{
  
}

void
EMEH264Decoder::InputDataExhausted()
{
  MOZ_ASSERT(IsOnGMPThread());
  mCallback->InputExhausted();
}

void
EMEH264Decoder::DrainComplete()
{
  MOZ_ASSERT(IsOnGMPThread());
  mCallback->DrainComplete();
}

void
EMEH264Decoder::ResetComplete()
{
  MOZ_ASSERT(IsOnGMPThread());
  {
    MonitorAutoLock mon(mMonitor);
    mFlushComplete = true;
    mon.NotifyAll();
  }
}

void
EMEH264Decoder::Error(GMPErr aErr)
{
  MOZ_ASSERT(IsOnGMPThread());
  EME_LOG("EMEH264Decoder::Error %d", aErr);
  if (aErr == GMPNoKeyErr) {
    
    
    NS_WARNING("GMP failed to decrypt due to lack of key");
  } else {
    mCallback->Error();
    GmpShutdown();
  }
}

void
EMEH264Decoder::Terminated()
{
  MOZ_ASSERT(IsOnGMPThread());

  NS_WARNING("H.264 GMP decoder terminated.");
  GmpShutdown();
}

nsresult
EMEH264Decoder::GmpInit()
{
  MOZ_ASSERT(IsOnGMPThread());

  nsTArray<nsCString> tags;
  tags.AppendElement(NS_LITERAL_CSTRING("h264"));
  tags.AppendElement(NS_ConvertUTF16toUTF8(mProxy->KeySystem()));
  nsresult rv = mMPS->GetGMPVideoDecoder(&tags,
                                         mProxy->GetNodeId(),
                                         &mHost,
                                         &mGMP);
  NS_ENSURE_SUCCESS(rv, rv);
  MOZ_ASSERT(mHost && mGMP);

  GMPVideoCodec codec;
  memset(&codec, 0, sizeof(codec));

  codec.mGMPApiVersion = kGMPVersion33;

  codec.mCodecType = kGMPVideoCodecH264;
  codec.mWidth = mConfig.display_width;
  codec.mHeight = mConfig.display_height;

  nsTArray<uint8_t> codecSpecific;
  codecSpecific.AppendElement(0); 
  codecSpecific.AppendElements(mConfig.extra_data->Elements(),
                               mConfig.extra_data->Length());

  rv = mGMP->InitDecode(codec,
                        codecSpecific,
                        this,
                        PR_GetNumberOfProcessors());
  NS_ENSURE_SUCCESS(rv, rv);

  mVideoInfo.mDisplay = nsIntSize(mConfig.display_width, mConfig.display_height);
  mVideoInfo.mHasVideo = true;

  return NS_OK;
}

nsresult
EMEH264Decoder::GmpInput(MP4Sample* aSample)
{
  MOZ_ASSERT(IsOnGMPThread());

  nsAutoPtr<MP4Sample> sample(aSample);
  if (!mGMP) {
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  mLastStreamOffset = sample->byte_offset;

  GMPVideoFrame* ftmp = nullptr;
  GMPErr err = mHost->CreateFrame(kGMPEncodedVideoFrame, &ftmp);
  if (GMP_FAILED(err)) {
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  UniquePtr<gmp::GMPVideoEncodedFrameImpl> frame(static_cast<gmp::GMPVideoEncodedFrameImpl*>(ftmp));
  err = frame->CreateEmptyFrame(sample->size);
  if (GMP_FAILED(err)) {
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  memcpy(frame->Buffer(), sample->data, frame->Size());

  frame->SetEncodedWidth(mConfig.display_width);
  frame->SetEncodedHeight(mConfig.display_height);
  frame->SetTimeStamp(sample->composition_timestamp);
  frame->SetCompleteFrame(true);
  frame->SetDuration(sample->duration);
  if (sample->crypto.valid) {
    frame->InitCrypto(sample->crypto);
  }
  frame->SetFrameType(sample->is_sync_point ? kGMPKeyFrame : kGMPDeltaFrame);
  frame->SetBufferType(GMP_BufferLength32);

  nsTArray<uint8_t> info; 
  nsresult rv = mGMP->Decode(UniquePtr<GMPVideoEncodedFrame>(frame.release()), false, info, 0);
  if (NS_FAILED(rv)) {
    mCallback->Error();
    return rv;
  }

  return NS_OK;
}

void
EMEH264Decoder::GmpFlush()
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mGMP || NS_FAILED(mGMP->Reset())) {
    
    MonitorAutoLock mon(mMonitor);
    mFlushComplete = true;
    mon.NotifyAll();
  }
}

void
EMEH264Decoder::GmpDrain()
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mGMP || NS_FAILED(mGMP->Drain())) {
    mCallback->DrainComplete();
  }
}

void
EMEH264Decoder::GmpShutdown()
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mGMP) {
    return;
  }
  mGMP->Close();
  mGMP = nullptr;
}

} 
