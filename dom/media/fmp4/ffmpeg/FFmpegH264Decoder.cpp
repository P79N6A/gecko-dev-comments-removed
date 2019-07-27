





#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "ImageContainer.h"

#include "mp4_demuxer/mp4_demuxer.h"

#include "FFmpegH264Decoder.h"

#define GECKO_FRAME_TYPE 0x00093CC0

typedef mozilla::layers::Image Image;
typedef mozilla::layers::PlanarYCbCrImage PlanarYCbCrImage;

namespace mozilla
{

FFmpegH264Decoder<LIBAV_VER>::FFmpegH264Decoder(
  FlushableMediaTaskQueue* aTaskQueue, MediaDataDecoderCallback* aCallback,
  const mp4_demuxer::VideoDecoderConfig& aConfig,
  ImageContainer* aImageContainer)
  : FFmpegDataDecoder(aTaskQueue, GetCodecId(aConfig.mime_type))
  , mCallback(aCallback)
  , mImageContainer(aImageContainer)
  , mDisplayWidth(aConfig.display_width)
  , mDisplayHeight(aConfig.display_height)
{
  MOZ_COUNT_CTOR(FFmpegH264Decoder);
  
  mExtraData = new DataBuffer;
  mExtraData->AppendElements(*aConfig.extra_data);
}

nsresult
FFmpegH264Decoder<LIBAV_VER>::Init()
{
  nsresult rv = FFmpegDataDecoder::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mCodecContext->get_buffer = AllocateBufferCb;
  mCodecContext->release_buffer = ReleaseBufferCb;

  return NS_OK;
}

FFmpegH264Decoder<LIBAV_VER>::DecodeResult
FFmpegH264Decoder<LIBAV_VER>::DoDecodeFrame(MediaRawData* aSample)
{
  AVPacket packet;
  av_init_packet(&packet);

  packet.data = const_cast<uint8_t*>(aSample->mData);
  packet.size = aSample->mSize;
  packet.dts = aSample->mTimecode;
  packet.pts = aSample->mTime;
  packet.flags = aSample->mKeyframe ? AV_PKT_FLAG_KEY : 0;
  packet.pos = aSample->mOffset;

  if (!PrepareFrame()) {
    NS_WARNING("FFmpeg h264 decoder failed to allocate frame.");
    mCallback->Error();
    return DecodeResult::DECODE_ERROR;
  }

  int decoded;
  int bytesConsumed =
    avcodec_decode_video2(mCodecContext, mFrame, &decoded, &packet);

  if (bytesConsumed < 0) {
    NS_WARNING("FFmpeg video decoder error.");
    mCallback->Error();
    return DecodeResult::DECODE_ERROR;
  }

  
  if (decoded) {
    VideoInfo info;
    info.mDisplay = nsIntSize(mDisplayWidth, mDisplayHeight);
    info.mStereoMode = StereoMode::MONO;
    info.mHasVideo = true;

    VideoData::YCbCrBuffer b;
    b.mPlanes[0].mData = mFrame->data[0];
    b.mPlanes[0].mStride = mFrame->linesize[0];
    b.mPlanes[0].mHeight = mFrame->height;
    b.mPlanes[0].mWidth = mFrame->width;
    b.mPlanes[0].mOffset = b.mPlanes[0].mSkip = 0;

    b.mPlanes[1].mData = mFrame->data[1];
    b.mPlanes[1].mStride = mFrame->linesize[1];
    b.mPlanes[1].mHeight = (mFrame->height + 1) >> 1;
    b.mPlanes[1].mWidth = (mFrame->width + 1) >> 1;
    b.mPlanes[1].mOffset = b.mPlanes[1].mSkip = 0;

    b.mPlanes[2].mData = mFrame->data[2];
    b.mPlanes[2].mStride = mFrame->linesize[2];
    b.mPlanes[2].mHeight = (mFrame->height + 1) >> 1;
    b.mPlanes[2].mWidth = (mFrame->width + 1) >> 1;
    b.mPlanes[2].mOffset = b.mPlanes[2].mSkip = 0;

    nsRefPtr<VideoData> v = VideoData::Create(info,
                                              mImageContainer,
                                              aSample->mOffset,
                                              mFrame->pkt_pts,
                                              aSample->mDuration,
                                              b,
                                              aSample->mKeyframe,
                                              -1,
                                              gfx::IntRect(0, 0, mCodecContext->width, mCodecContext->height));
    if (!v) {
      NS_WARNING("image allocation error.");
      mCallback->Error();
      return DecodeResult::DECODE_ERROR;
    }
    mCallback->Output(v);
    return DecodeResult::DECODE_FRAME;
  }
  return DecodeResult::DECODE_NO_FRAME;
}

void
FFmpegH264Decoder<LIBAV_VER>::DecodeFrame(MediaRawData* aSample)
{
  if (DoDecodeFrame(aSample) != DecodeResult::DECODE_ERROR &&
      mTaskQueue->IsEmpty()) {
    mCallback->InputExhausted();
  }
}

 int
FFmpegH264Decoder<LIBAV_VER>::AllocateBufferCb(AVCodecContext* aCodecContext,
                                               AVFrame* aFrame)
{
  MOZ_ASSERT(aCodecContext->codec_type == AVMEDIA_TYPE_VIDEO);

  FFmpegH264Decoder* self =
    static_cast<FFmpegH264Decoder*>(aCodecContext->opaque);

  switch (aCodecContext->pix_fmt) {
  case PIX_FMT_YUV420P:
    return self->AllocateYUV420PVideoBuffer(aCodecContext, aFrame);
  default:
    return avcodec_default_get_buffer(aCodecContext, aFrame);
  }
}

 void
FFmpegH264Decoder<LIBAV_VER>::ReleaseBufferCb(AVCodecContext* aCodecContext,
                                              AVFrame* aFrame)
{
  switch (aCodecContext->pix_fmt) {
    case PIX_FMT_YUV420P: {
      Image* image = static_cast<Image*>(aFrame->opaque);
      if (image) {
        image->Release();
      }
      break;
    }
    default:
      avcodec_default_release_buffer(aCodecContext, aFrame);
      break;
  }
}

int
FFmpegH264Decoder<LIBAV_VER>::AllocateYUV420PVideoBuffer(
  AVCodecContext* aCodecContext, AVFrame* aFrame)
{
  bool needAlign = aCodecContext->codec->capabilities & CODEC_CAP_DR1;
  int edgeWidth =  needAlign ? avcodec_get_edge_width() : 0;
  int decodeWidth = aCodecContext->width + edgeWidth * 2;
  
  
  
  decodeWidth = (decodeWidth + 31) & ~31;
  int decodeHeight = aCodecContext->height + edgeWidth * 2;

  if (needAlign) {
    
    int stride_align[AV_NUM_DATA_POINTERS];
    avcodec_align_dimensions2(aCodecContext, &decodeWidth, &decodeHeight,
                              stride_align);
  }

  
  av_image_fill_linesizes(aFrame->linesize, aCodecContext->pix_fmt,
                          decodeWidth);

  
  
  
  
  size_t allocSize =
    av_image_fill_pointers(aFrame->data, aCodecContext->pix_fmt, decodeHeight,
                           nullptr , aFrame->linesize);

  nsRefPtr<Image> image =
    mImageContainer->CreateImage(ImageFormat::PLANAR_YCBCR);
  PlanarYCbCrImage* ycbcr = static_cast<PlanarYCbCrImage*>(image.get());
  uint8_t* buffer = ycbcr->AllocateAndGetNewBuffer(allocSize + 64);
  
  buffer = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(buffer) + 63) & ~63);

  if (!buffer) {
    NS_WARNING("Failed to allocate buffer for FFmpeg video decoding");
    return -1;
  }

  
  
  
  for (uint32_t i = 0; i < AV_NUM_DATA_POINTERS; i++) {
    
    
    uint32_t planeEdgeWidth = edgeWidth / (i ? 2 : 1);

    
    
    aFrame->data[i] += reinterpret_cast<ptrdiff_t>(
      buffer + planeEdgeWidth * aFrame->linesize[i] + planeEdgeWidth);
  }

  
  aFrame->type = GECKO_FRAME_TYPE;

  aFrame->extended_data = aFrame->data;
  aFrame->width = aCodecContext->width;
  aFrame->height = aCodecContext->height;

  aFrame->opaque = static_cast<void*>(image.forget().take());

  return 0;
}

nsresult
FFmpegH264Decoder<LIBAV_VER>::Input(MediaRawData* aSample)
{
  mTaskQueue->Dispatch(
    NS_NewRunnableMethodWithArg<nsRefPtr<MediaRawData>>(
      this, &FFmpegH264Decoder<LIBAV_VER>::DecodeFrame,
      nsRefPtr<MediaRawData>(aSample)));

  return NS_OK;
}

void
FFmpegH264Decoder<LIBAV_VER>::DoDrain()
{
  nsRefPtr<MediaRawData> empty(new MediaRawData());
  while (DoDecodeFrame(empty) == DecodeResult::DECODE_FRAME) {
  }
  mCallback->DrainComplete();
}

nsresult
FFmpegH264Decoder<LIBAV_VER>::Drain()
{
  mTaskQueue->Dispatch(
    NS_NewRunnableMethod(this, &FFmpegH264Decoder<LIBAV_VER>::DoDrain));

  return NS_OK;
}

nsresult
FFmpegH264Decoder<LIBAV_VER>::Flush()
{
  nsresult rv = FFmpegDataDecoder::Flush();
  
  return rv;
}

FFmpegH264Decoder<LIBAV_VER>::~FFmpegH264Decoder()
{
  MOZ_COUNT_DTOR(FFmpegH264Decoder);
}

AVCodecID
FFmpegH264Decoder<LIBAV_VER>::GetCodecId(const nsACString& aMimeType)
{
  if (aMimeType.EqualsLiteral("video/avc") || aMimeType.EqualsLiteral("video/mp4")) {
    return AV_CODEC_ID_H264;
  }

  if (aMimeType.EqualsLiteral("video/x-vnd.on2.vp6")) {
    return AV_CODEC_ID_VP6F;
  }

  return AV_CODEC_ID_NONE;
}

} 
