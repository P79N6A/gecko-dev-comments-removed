




#include "VP8TrackEncoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_encoder.h"
#include "VideoSegment.h"
#include "VideoUtils.h"
#include "prsystem.h"
#include "WebMWriter.h"
#include "libyuv.h"

namespace mozilla {

#ifdef PR_LOGGING
PRLogModuleInfo* gVP8TrackEncoderLog;
#define VP8LOG(msg, ...) PR_LOG(gVP8TrackEncoderLog, PR_LOG_DEBUG, \
                                  (msg, ##__VA_ARGS__))

#else
#define VP8LOG(msg, ...)
#endif

#define DEFAULT_BITRATE 2500 // in kbit/s
#define DEFAULT_ENCODE_FRAMERATE 30

using namespace mozilla::layers;

VP8TrackEncoder::VP8TrackEncoder()
  : VideoTrackEncoder()
  , mEncodedFrameDuration(0)
  , mEncodedTimestamp(0)
  , mRemainingTicks(0)
  , mVPXContext(new vpx_codec_ctx_t())
  , mVPXImageWrapper(new vpx_image_t())
{
  MOZ_COUNT_CTOR(VP8TrackEncoder);
#ifdef PR_LOGGING
  if (!gVP8TrackEncoderLog) {
    gVP8TrackEncoderLog = PR_NewLogModule("VP8TrackEncoder");
  }
#endif
}

VP8TrackEncoder::~VP8TrackEncoder()
{
  if (mInitialized) {
    vpx_codec_destroy(mVPXContext);
  }

  if (mVPXImageWrapper) {
    vpx_img_free(mVPXImageWrapper);
  }
  MOZ_COUNT_DTOR(VP8TrackEncoder);
}

nsresult
VP8TrackEncoder::Init(int32_t aWidth, int32_t aHeight, int32_t aDisplayWidth,
                      int32_t aDisplayHeight,TrackRate aTrackRate)
{
  if (aWidth < 1 || aHeight < 1 || aDisplayWidth < 1 || aDisplayHeight < 1
      || aTrackRate <= 0) {
    return NS_ERROR_FAILURE;
  }

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mTrackRate = aTrackRate;
  mEncodedFrameRate = DEFAULT_ENCODE_FRAMERATE;
  mEncodedFrameDuration = mTrackRate / mEncodedFrameRate;
  mFrameWidth = aWidth;
  mFrameHeight = aHeight;
  mDisplayWidth = aDisplayWidth;
  mDisplayHeight = aDisplayHeight;

  
  vpx_codec_enc_cfg_t config;
  memset(&config, 0, sizeof(vpx_codec_enc_cfg_t));
  if (vpx_codec_enc_config_default(vpx_codec_vp8_cx(), &config, 0)) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  vpx_img_wrap(mVPXImageWrapper, VPX_IMG_FMT_I420,
               mFrameWidth, mFrameHeight, 1, nullptr);

  config.g_w = mFrameWidth;
  config.g_h = mFrameHeight;
  
  
  config.rc_target_bitrate = DEFAULT_BITRATE; 

  
  config.g_timebase.num = 1;
  config.g_timebase.den = mTrackRate;

  config.g_error_resilient = 0;

  config.g_lag_in_frames = 0; 

  int32_t number_of_cores = PR_GetNumberOfProcessors();
  if (mFrameWidth * mFrameHeight > 1280 * 960 && number_of_cores >= 6) {
    config.g_threads = 3; 
  } else if (mFrameWidth * mFrameHeight > 640 * 480 && number_of_cores >= 3) {
    config.g_threads = 2; 
  } else {
    config.g_threads = 1; 
  }

  
  config.rc_dropframe_thresh = 0;
  config.rc_end_usage = VPX_CBR;
  config.g_pass = VPX_RC_ONE_PASS;
  config.rc_resize_allowed = 1;
  config.rc_undershoot_pct = 100;
  config.rc_overshoot_pct = 15;
  config.rc_buf_initial_sz = 500;
  config.rc_buf_optimal_sz = 600;
  config.rc_buf_sz = 1000;

  config.kf_mode = VPX_KF_AUTO;
  
  config.kf_max_dist = mEncodedFrameRate;

  vpx_codec_flags_t flags = 0;
  flags |= VPX_CODEC_USE_OUTPUT_PARTITION;
  if (vpx_codec_enc_init(mVPXContext, vpx_codec_vp8_cx(), &config, flags)) {
    return NS_ERROR_FAILURE;
  }

  vpx_codec_control(mVPXContext, VP8E_SET_STATIC_THRESHOLD, 1);
  vpx_codec_control(mVPXContext, VP8E_SET_CPUUSED, -6);
  vpx_codec_control(mVPXContext, VP8E_SET_TOKEN_PARTITIONS,
                    VP8_ONE_TOKENPARTITION);

  mInitialized = true;
  mon.NotifyAll();

  return NS_OK;
}

already_AddRefed<TrackMetadataBase>
VP8TrackEncoder::GetMetadata()
{
  {
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    while (!mCanceled && !mInitialized) {
      mon.Wait();
    }
  }

  if (mCanceled || mEncodingComplete) {
    return nullptr;
  }

  nsRefPtr<VP8Metadata> meta = new VP8Metadata();
  meta->mWidth = mFrameWidth;
  meta->mHeight = mFrameHeight;
  meta->mDisplayWidth = mDisplayWidth;
  meta->mDisplayHeight = mDisplayHeight;
  meta->mEncodedFrameRate = mEncodedFrameRate;

  return meta.forget();
}

nsresult
VP8TrackEncoder::GetEncodedPartitions(EncodedFrameContainer& aData)
{
  vpx_codec_iter_t iter = nullptr;
  EncodedFrame::FrameType frameType = EncodedFrame::VP8_P_FRAME;
  nsTArray<uint8_t> frameData;
  const vpx_codec_cx_pkt_t *pkt = nullptr;
  while ((pkt = vpx_codec_get_cx_data(mVPXContext, &iter)) != nullptr) {
    switch (pkt->kind) {
      case VPX_CODEC_CX_FRAME_PKT: {
        
        frameData.AppendElements((uint8_t*)pkt->data.frame.buf,
                                 pkt->data.frame.sz);
        break;
      }
      default: {
        break;
      }
    }
    
    if ((pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT) == 0) {
      if (pkt->data.frame.flags & VPX_FRAME_IS_KEY) {
        frameType = EncodedFrame::VP8_I_FRAME;
      }
      break;
    }
  }

  if (!frameData.IsEmpty() &&
      (pkt->data.frame.pts == mEncodedTimestamp)) {
    
    EncodedFrame* videoData = new EncodedFrame();
    videoData->SetFrameType(frameType);
    
    CheckedInt64 timestamp = FramesToUsecs(mEncodedTimestamp, mTrackRate);
    if (timestamp.isValid()) {
      videoData->SetTimeStamp(
        (uint64_t)FramesToUsecs(mEncodedTimestamp, mTrackRate).value());
    }
    CheckedInt64 duration = FramesToUsecs(pkt->data.frame.duration, mTrackRate);
    if (duration.isValid()) {
      videoData->SetDuration(
        (uint64_t)FramesToUsecs(pkt->data.frame.duration, mTrackRate).value());
    }
    videoData->SwapInFrameData(frameData);
    VP8LOG("GetEncodedPartitions TimeStamp %lld Duration %lld\n",
           videoData->GetTimeStamp(), videoData->GetDuration());
    VP8LOG("frameType %d\n", videoData->GetFrameType());
    aData.AppendEncodedFrame(videoData);
  }

  return NS_OK;
}

static bool isYUV420(const PlanarYCbCrImage::Data *aData)
{
  if (aData->mYSize == aData->mCbCrSize * 2) {
    return true;
  }
  return false;
}

static bool isYUV422(const PlanarYCbCrImage::Data *aData)
{
  if ((aData->mYSize.width == aData->mCbCrSize.width * 2) &&
      (aData->mYSize.height == aData->mCbCrSize.height)) {
    return true;
  }
  return false;
}

static bool isYUV444(const PlanarYCbCrImage::Data *aData)
{
  if (aData->mYSize == aData->mCbCrSize) {
    return true;
  }
  return false;
}

nsresult VP8TrackEncoder::PrepareRawFrame(VideoChunk &aChunk)
{
  nsRefPtr<Image> img;
  if (aChunk.mFrame.GetForceBlack() || aChunk.IsNull()) {
    if (!mMuteFrame) {
      mMuteFrame = VideoFrame::CreateBlackImage(gfxIntSize(mFrameWidth, mFrameHeight));
      MOZ_ASSERT(mMuteFrame);
    }
    img = mMuteFrame;
  } else {
    img = aChunk.mFrame.GetImage();
  }

  ImageFormat format = img->GetFormat();
  if (format != ImageFormat::PLANAR_YCBCR) {
    VP8LOG("Unsupported video format\n");
    return NS_ERROR_FAILURE;
  }

  
  PlanarYCbCrImage* yuv =
  const_cast<PlanarYCbCrImage *>(static_cast<const PlanarYCbCrImage *>(img.get()));
  
  
  MOZ_ASSERT(yuv);
  if (!yuv->IsValid()) {
    NS_WARNING("PlanarYCbCrImage is not valid");
    return NS_ERROR_FAILURE;
  }
  const PlanarYCbCrImage::Data *data = yuv->GetData();

  if (isYUV420(data) && !data->mCbSkip) { 
    mVPXImageWrapper->planes[VPX_PLANE_Y] = data->mYChannel;
    mVPXImageWrapper->planes[VPX_PLANE_U] = data->mCbChannel;
    mVPXImageWrapper->planes[VPX_PLANE_V] = data->mCrChannel;
    mVPXImageWrapper->stride[VPX_PLANE_Y] = data->mYStride;
    mVPXImageWrapper->stride[VPX_PLANE_U] = data->mCbCrStride;
    mVPXImageWrapper->stride[VPX_PLANE_V] = data->mCbCrStride;
  } else {
    uint32_t yPlaneSize = mFrameWidth * mFrameHeight;
    uint32_t halfWidth = (mFrameWidth + 1) / 2;
    uint32_t halfHeight = (mFrameHeight + 1) / 2;
    uint32_t uvPlaneSize = halfWidth * halfHeight;
    if (mI420Frame.IsEmpty()) {
      mI420Frame.SetLength(yPlaneSize + uvPlaneSize * 2);
    }

    MOZ_ASSERT(mI420Frame.Length() >= (yPlaneSize + uvPlaneSize * 2));
    uint8_t *y = mI420Frame.Elements();
    uint8_t *cb = mI420Frame.Elements() + yPlaneSize;
    uint8_t *cr = mI420Frame.Elements() + yPlaneSize + uvPlaneSize;

    if (isYUV420(data) && data->mCbSkip) {
      
      if (data->mCbChannel < data->mCrChannel) { 
        libyuv::NV12ToI420(data->mYChannel, data->mYStride,
                           data->mCbChannel, data->mCbCrStride,
                           y, mFrameWidth,
                           cb, halfWidth,
                           cr, halfWidth,
                           mFrameWidth, mFrameHeight);
      } else { 
        libyuv::NV21ToI420(data->mYChannel, data->mYStride,
                           data->mCrChannel, data->mCbCrStride,
                           y, mFrameWidth,
                           cb, halfWidth,
                           cr, halfWidth,
                           mFrameWidth, mFrameHeight);
      }
    } else if (isYUV444(data) && !data->mCbSkip) {
      libyuv::I444ToI420(data->mYChannel, data->mYStride,
                         data->mCbChannel, data->mCbCrStride,
                         data->mCrChannel, data->mCbCrStride,
                         y, mFrameWidth,
                         cb, halfWidth,
                         cr, halfWidth,
                         mFrameWidth, mFrameHeight);
    } else if (isYUV422(data) && !data->mCbSkip) {
      libyuv::I422ToI420(data->mYChannel, data->mYStride,
                         data->mCbChannel, data->mCbCrStride,
                         data->mCrChannel, data->mCbCrStride,
                         y, mFrameWidth,
                         cb, halfWidth,
                         cr, halfWidth,
                         mFrameWidth, mFrameHeight);
    } else {
      VP8LOG("Unsupported planar format\n");
      return NS_ERROR_NOT_IMPLEMENTED;
    }

    mVPXImageWrapper->planes[VPX_PLANE_Y] = y;
    mVPXImageWrapper->planes[VPX_PLANE_U] = cb;
    mVPXImageWrapper->planes[VPX_PLANE_V] = cr;
    mVPXImageWrapper->stride[VPX_PLANE_Y] = mFrameWidth;
    mVPXImageWrapper->stride[VPX_PLANE_U] = halfWidth;
    mVPXImageWrapper->stride[VPX_PLANE_V] = halfWidth;
  }

  return NS_OK;
}



#define I_FRAME_RATIO (0.5)
#define SKIP_FRAME_RATIO (0.75)






VP8TrackEncoder::EncodeOperation
VP8TrackEncoder::GetNextEncodeOperation(TimeDuration aTimeElapsed,
                                        TrackTicks aProcessedDuration)
{
  int64_t durationInUsec =
    FramesToUsecs(aProcessedDuration + mEncodedFrameDuration,
                  mTrackRate).value();
  if (aTimeElapsed.ToMicroseconds() > (durationInUsec * SKIP_FRAME_RATIO)) {
    
    
    return SKIP_FRAME;
  } else if (aTimeElapsed.ToMicroseconds() > (durationInUsec * I_FRAME_RATIO)) {
    
    
    return ENCODE_I_FRAME;
  } else {
    return ENCODE_NORMAL_FRAME;
  }
}

TrackTicks
VP8TrackEncoder::CalculateRemainingTicks(TrackTicks aDurationCopied,
                                         TrackTicks aEncodedDuration)
{
  return mRemainingTicks + aEncodedDuration - aDurationCopied;
}



TrackTicks
VP8TrackEncoder::CalculateEncodedDuration(TrackTicks aDurationCopied)
{
  TrackTicks temp64 = aDurationCopied;
  TrackTicks encodedDuration = mEncodedFrameDuration;
  temp64 -= mRemainingTicks;
  while (temp64 > mEncodedFrameDuration) {
    temp64 -= mEncodedFrameDuration;
    encodedDuration += mEncodedFrameDuration;
  }
  return encodedDuration;
}













































nsresult
VP8TrackEncoder::GetEncodedTrack(EncodedFrameContainer& aData)
{
  {
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    
    
    while (!mCanceled && (!mInitialized ||
           (mRawSegment.GetDuration() + mSourceSegment.GetDuration() <
            mEncodedFrameDuration && !mEndOfStream))) {
      mon.Wait();
    }
    if (mCanceled || mEncodingComplete) {
      return NS_ERROR_FAILURE;
    }
    mSourceSegment.AppendFrom(&mRawSegment);
  }

  VideoSegment::ChunkIterator iter(mSourceSegment);
  TrackTicks durationCopied = 0;
  TrackTicks totalProcessedDuration = 0;
  TimeStamp timebase = TimeStamp::Now();
  EncodeOperation nextEncodeOperation = ENCODE_NORMAL_FRAME;

  for (; !iter.IsEnded(); iter.Next()) {
    VideoChunk &chunk = *iter;
    
    
    durationCopied += chunk.GetDuration();
    MOZ_ASSERT(mRemainingTicks <= mEncodedFrameDuration);
    VP8LOG("durationCopied %lld mRemainingTicks %lld\n",
           durationCopied, mRemainingTicks);
    if (durationCopied >= mRemainingTicks) {
      VP8LOG("nextEncodeOperation is %d\n",nextEncodeOperation);
      
      TrackTicks encodedDuration = CalculateEncodedDuration(durationCopied);

      
      if (nextEncodeOperation != SKIP_FRAME) {
        nsresult rv = PrepareRawFrame(chunk);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

        
        int flags = (nextEncodeOperation == ENCODE_NORMAL_FRAME) ?
                    0 : VPX_EFLAG_FORCE_KF;
        if (vpx_codec_encode(mVPXContext, mVPXImageWrapper, mEncodedTimestamp,
                             (unsigned long)encodedDuration, flags,
                             VPX_DL_REALTIME)) {
          return NS_ERROR_FAILURE;
        }
        
        GetEncodedPartitions(aData);
      } else {
        
        
        
        nsRefPtr<EncodedFrame> last = nullptr;
        last = aData.GetEncodedFrames().LastElement();
        if (last) {
          last->SetDuration(last->GetDuration() + encodedDuration);
        }
      }
      
      mEncodedTimestamp += encodedDuration;
      totalProcessedDuration += durationCopied;
      
      mRemainingTicks = CalculateRemainingTicks(durationCopied,
                                                encodedDuration);

      
      if (mSourceSegment.GetDuration() - totalProcessedDuration
          >= mEncodedFrameDuration) {
        TimeDuration elapsedTime = TimeStamp::Now() - timebase;
        nextEncodeOperation = GetNextEncodeOperation(elapsedTime,
                                                     totalProcessedDuration);
        
        durationCopied = 0;
      } else {
        
        
        break;
      }
    }
  }
  
  mSourceSegment.RemoveLeading(totalProcessedDuration);
  VP8LOG("RemoveLeading %lld\n",totalProcessedDuration);

  
  if (mEndOfStream) {
    VP8LOG("mEndOfStream is true\n");
    mEncodingComplete = true;
    if (vpx_codec_encode(mVPXContext, nullptr, mEncodedTimestamp,
                         mEncodedFrameDuration, 0, VPX_DL_REALTIME)) {
      return NS_ERROR_FAILURE;
    }
    GetEncodedPartitions(aData);
  }

  return NS_OK ;
}

} 
