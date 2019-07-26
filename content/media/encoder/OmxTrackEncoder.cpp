




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

}
