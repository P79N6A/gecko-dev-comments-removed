




#include "OmxTrackEncoder.h"
#include "OMXCodecWrapper.h"
#include "VideoUtils.h"
#include "ISOTrackMetadata.h"

#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define OMX_LOG(args...)                                                       \
  do {                                                                         \
    __android_log_print(ANDROID_LOG_INFO, "OmxTrackEncoder", ##args);          \
  } while (0)
#else
#define OMX_LOG(args, ...)
#endif

using namespace android;

namespace mozilla {

#define ENCODER_CONFIG_FRAME_RATE 30 // fps
#define GET_ENCODED_VIDEO_FRAME_TIMEOUT 100000 // microseconds

nsresult
OmxVideoTrackEncoder::Init(int aWidth, int aHeight, TrackRate aTrackRate)
{
  mFrameWidth = aWidth;
  mFrameHeight = aHeight;
  mTrackRate = aTrackRate;

  mEncoder = OMXCodecWrapper::CreateAVCEncoder();
  NS_ENSURE_TRUE(mEncoder, NS_ERROR_FAILURE);

  nsresult rv = mEncoder->Configure(mFrameWidth, mFrameHeight,
                                    ENCODER_CONFIG_FRAME_RATE);

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mInitialized = (rv == NS_OK);

  mReentrantMonitor.NotifyAll();

  return rv;
}

already_AddRefed<TrackMetadataBase>
OmxVideoTrackEncoder::GetMetadata()
{
  {
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    while (!mCanceled && !mInitialized) {
      mReentrantMonitor.Wait();
    }
  }

  if (mCanceled || mEncodingComplete) {
    return nullptr;
  }

  nsRefPtr<AVCTrackMetadata> meta = new AVCTrackMetadata();
  meta->Width = mFrameWidth;
  meta->Height = mFrameHeight;
  meta->FrameRate = ENCODER_CONFIG_FRAME_RATE;
  meta->VideoFrequency = 90000; 
  return meta.forget();
}

nsresult
OmxVideoTrackEncoder::GetEncodedTrack(EncodedFrameContainer& aData)
{
  VideoSegment segment;
  {
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    while (!mCanceled && (!mInitialized ||
          (mRawSegment.GetDuration() == 0 && !mEndOfStream))) {
      mReentrantMonitor.Wait();
    }

    if (mCanceled || mEncodingComplete) {
      return NS_ERROR_FAILURE;
    }

    segment.AppendFrom(&mRawSegment);
  }

  
  VideoSegment::ChunkIterator iter(segment);
  while (!iter.IsEnded()) {
    VideoChunk chunk = *iter;

    
    if (mLastFrame != chunk.mFrame) {
      uint64_t totalDurationUs = mTotalFrameDuration * USECS_PER_S / mTrackRate;
      layers::Image* img = (chunk.IsNull() || chunk.mFrame.GetForceBlack()) ?
                           nullptr : chunk.mFrame.GetImage();
      mEncoder->Encode(img, mFrameWidth, mFrameHeight, totalDurationUs);
    }

    mLastFrame.TakeFrom(&chunk.mFrame);
    mTotalFrameDuration += chunk.GetDuration();

    iter.Next();
  }

  
  if (mEndOfStream && iter.IsEnded() && !mEosSetInEncoder) {
    mEosSetInEncoder = true;
    uint64_t totalDurationUs = mTotalFrameDuration * USECS_PER_S / mTrackRate;
    layers::Image* img = (!mLastFrame.GetImage() || mLastFrame.GetForceBlack())
                         ? nullptr : mLastFrame.GetImage();
    mEncoder->Encode(img, mFrameWidth, mFrameHeight, totalDurationUs,
                     OMXCodecWrapper::BUFFER_EOS);
  }

  
  nsTArray<uint8_t> buffer;
  int outFlags = 0;
  int64_t outTimeStampUs = 0;
  mEncoder->GetNextEncodedFrame(&buffer, &outTimeStampUs, &outFlags,
                                GET_ENCODED_VIDEO_FRAME_TIMEOUT);
  if (!buffer.IsEmpty()) {
    nsRefPtr<EncodedFrame> videoData = new EncodedFrame();
    if (outFlags & OMXCodecWrapper::BUFFER_CODEC_CONFIG) {
      videoData->SetFrameType(EncodedFrame::AVC_CSD);
    } else {
      videoData->SetFrameType((outFlags & OMXCodecWrapper::BUFFER_SYNC_FRAME) ?
                              EncodedFrame::I_FRAME : EncodedFrame::P_FRAME);
    }
    videoData->SetFrameData(&buffer);
    videoData->SetTimeStamp(outTimeStampUs);
    aData.AppendEncodedFrame(videoData);
  }

  if (outFlags & OMXCodecWrapper::BUFFER_EOS) {
    mEncodingComplete = true;
    OMX_LOG("Done encoding video.");
  }

  return NS_OK;
}

nsresult
OmxAudioTrackEncoder::Init(int aChannels, int aSamplingRate)
{
  mChannels = aChannels;
  mSamplingRate = aSamplingRate;

  mEncoder = OMXCodecWrapper::CreateAACEncoder();
  NS_ENSURE_TRUE(mEncoder, NS_ERROR_FAILURE);

  nsresult rv = mEncoder->Configure(mChannels, mSamplingRate);

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mInitialized = (rv == NS_OK);

  mReentrantMonitor.NotifyAll();

  return NS_OK;
}

already_AddRefed<TrackMetadataBase>
OmxAudioTrackEncoder::GetMetadata()
{
  {
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    while (!mCanceled && !mInitialized) {
      mReentrantMonitor.Wait();
    }
  }

  if (mCanceled || mEncodingComplete) {
    return nullptr;
  }

  nsRefPtr<AACTrackMetadata> meta = new AACTrackMetadata();
  meta->Channels = mChannels;
  meta->SampleRate = mSamplingRate;
  meta->FrameSize = OMXCodecWrapper::kAACFrameSize;
  meta->FrameDuration = OMXCodecWrapper::kAACFrameDuration;

  return meta.forget();
}

nsresult
OmxAudioTrackEncoder::AppendEncodedFrames(EncodedFrameContainer& aContainer)
{
  nsTArray<uint8_t> frameData;
  int outFlags = 0;
  int64_t outTimeUs = -1;

  nsresult rv = mEncoder->GetNextEncodedFrame(&frameData, &outTimeUs, &outFlags,
                                              3000); 
  NS_ENSURE_SUCCESS(rv, rv);

  if (!frameData.IsEmpty()) {
    bool isCSD = false;
    if (outFlags & OMXCodecWrapper::BUFFER_CODEC_CONFIG) { 
      isCSD = true;
    } else if (outFlags & OMXCodecWrapper::BUFFER_EOS) { 
      mEncodingComplete = true;
    } else {
      MOZ_ASSERT(frameData.Length() == OMXCodecWrapper::kAACFrameSize);
    }

    nsRefPtr<EncodedFrame> audiodata = new EncodedFrame();
    audiodata->SetFrameType(isCSD ?
      EncodedFrame::AAC_CSD : EncodedFrame::AUDIO_FRAME);
    audiodata->SetTimeStamp(outTimeUs);
    audiodata->SetFrameData(&frameData);
    aContainer.AppendEncodedFrame(audiodata);
  }

  return NS_OK;
}

nsresult
OmxAudioTrackEncoder::GetEncodedTrack(EncodedFrameContainer& aData)
{
  AudioSegment segment;
  
  
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    while (!mInitialized && !mCanceled) {
      mReentrantMonitor.Wait();
    }

    if (mCanceled || mEncodingComplete) {
      return NS_ERROR_FAILURE;
    }

    segment.AppendFrom(&mRawSegment);
  }

  if (!mEosSetInEncoder) {
    if (mEndOfStream) {
      mEosSetInEncoder = true;
    }
    if (segment.GetDuration() > 0 || mEndOfStream) {
      
      nsresult rv = mEncoder->Encode(segment,
                                mEndOfStream ? OMXCodecWrapper::BUFFER_EOS : 0);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return AppendEncodedFrames(aData);
}

}
