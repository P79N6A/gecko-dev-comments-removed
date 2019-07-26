





#include "MediaTaskQueue.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "ImageContainer.h"

#include "mp4_demuxer/mp4_demuxer.h"
#include "FFmpegRuntimeLinker.h"

#include "FFmpegH264Decoder.h"

#define GECKO_FRAME_TYPE 0x00093CC0

typedef mozilla::layers::Image Image;
typedef mozilla::layers::PlanarYCbCrImage PlanarYCbCrImage;

typedef mp4_demuxer::MP4Sample MP4Sample;

namespace mozilla
{

FFmpegH264Decoder::FFmpegH264Decoder(
  MediaTaskQueue* aTaskQueue, MediaDataDecoderCallback* aCallback,
  const mp4_demuxer::VideoDecoderConfig &aConfig,
  ImageContainer* aImageContainer)
  : FFmpegDataDecoder(aTaskQueue, AV_CODEC_ID_H264)
  , mCallback(aCallback)
  , mImageContainer(aImageContainer)
{
  MOZ_COUNT_CTOR(FFmpegH264Decoder);
}

nsresult
FFmpegH264Decoder::Init()
{
  nsresult rv = FFmpegDataDecoder::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mCodecContext.get_buffer = AllocateBufferCb;

  return NS_OK;
}

void
FFmpegH264Decoder::DecodeFrame(mp4_demuxer::MP4Sample* aSample)
{
  AVPacket packet;
  av_init_packet(&packet);

  packet.data = aSample->data;
  packet.size = aSample->size;
  packet.pts = aSample->composition_timestamp;
  packet.flags = aSample->is_sync_point ? AV_PKT_FLAG_KEY : 0;
  packet.pos = aSample->byte_offset;

  nsAutoPtr<AVFrame> frame(avcodec_alloc_frame());
  avcodec_get_frame_defaults(frame);

  int decoded;
  int bytesConsumed =
    avcodec_decode_video2(&mCodecContext, frame, &decoded, &packet);

  if (bytesConsumed < 0) {
    NS_WARNING("FFmpeg video decoder error.");
    mCallback->Error();
    return;
  }

  if (!decoded) {
    
    return;
  }

  nsAutoPtr<VideoData> data;

  VideoInfo info;
  info.mDisplay = nsIntSize(mCodecContext.width, mCodecContext.height);
  info.mStereoMode = StereoMode::MONO;
  info.mHasVideo = true;

  data = VideoData::CreateFromImage(
    info, mImageContainer, aSample->byte_offset, aSample->composition_timestamp,
    aSample->duration, mCurrentImage, aSample->is_sync_point, -1,
    gfx::IntRect(0, 0, mCodecContext.width, mCodecContext.height));

  
  mDelayedFrames.Push(data.forget());

  
  
  
  if (mDelayedFrames.Length() > (uint32_t)mCodecContext.max_b_frames + 1) {
    VideoData* d = mDelayedFrames.Pop();
    mCallback->Output(d);
  }

  if (mTaskQueue->IsEmpty()) {
    mCallback->InputExhausted();
  }
}

static void
PlanarYCbCrDataFromAVFrame(mozilla::layers::PlanarYCbCrData &aData,
                           AVFrame* aFrame)
{
  aData.mPicX = aData.mPicY = 0;
  aData.mPicSize = mozilla::gfx::IntSize(aFrame->width, aFrame->height);
  aData.mStereoMode = StereoMode::MONO;

  aData.mYChannel = aFrame->data[0];
  aData.mYStride = aFrame->linesize[0];
  aData.mYSize = aData.mPicSize;
  aData.mYSkip = 0;

  aData.mCbChannel = aFrame->data[1];
  aData.mCrChannel = aFrame->data[2];
  aData.mCbCrStride = aFrame->linesize[1];
  aData.mCbSkip = aData.mCrSkip = 0;
  aData.mCbCrSize =
    mozilla::gfx::IntSize((aFrame->width + 1) / 2, (aFrame->height + 1) / 2);
}

 int
FFmpegH264Decoder::AllocateBufferCb(AVCodecContext* aCodecContext,
                                    AVFrame* aFrame)
{
  MOZ_ASSERT(aCodecContext->codec_type == AVMEDIA_TYPE_VIDEO);

  FFmpegH264Decoder* self =
    reinterpret_cast<FFmpegH264Decoder*>(aCodecContext->opaque);

  switch (aCodecContext->pix_fmt) {
  case PIX_FMT_YUV420P:
    return self->AllocateYUV420PVideoBuffer(aCodecContext, aFrame);
  default:
    return avcodec_default_get_buffer(aCodecContext, aFrame);
  }
}

int
FFmpegH264Decoder::AllocateYUV420PVideoBuffer(AVCodecContext* aCodecContext,
                                              AVFrame* aFrame)
{
  
  
  int edgeWidth = avcodec_get_edge_width();
  int decodeWidth = aCodecContext->width + edgeWidth * 2;
  int decodeHeight = aCodecContext->height + edgeWidth * 2;

  
  int stride_align[AV_NUM_DATA_POINTERS];
  avcodec_align_dimensions2(aCodecContext, &decodeWidth, &decodeHeight,
                            stride_align);

  
  av_image_fill_linesizes(aFrame->linesize, aCodecContext->pix_fmt,
                          decodeWidth);

  
  
  
  
  size_t allocSize =
    av_image_fill_pointers(aFrame->data, aCodecContext->pix_fmt, decodeHeight,
                           nullptr , aFrame->linesize);

  nsRefPtr<Image> image =
    mImageContainer->CreateImage(ImageFormat::PLANAR_YCBCR);
  PlanarYCbCrImage* ycbcr = reinterpret_cast<PlanarYCbCrImage*>(image.get());
  uint8_t* buffer = ycbcr->AllocateAndGetNewBuffer(allocSize);

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

  mozilla::layers::PlanarYCbCrData data;
  PlanarYCbCrDataFromAVFrame(data, aFrame);
  ycbcr->SetDataNoCopy(data);

  mCurrentImage.swap(image);

  return 0;
}

nsresult
FFmpegH264Decoder::Input(mp4_demuxer::MP4Sample* aSample)
{
  mTaskQueue->Dispatch(
    NS_NewRunnableMethodWithArg<nsAutoPtr<mp4_demuxer::MP4Sample> >(
      this, &FFmpegH264Decoder::DecodeFrame,
      nsAutoPtr<mp4_demuxer::MP4Sample>(aSample)));

  return NS_OK;
}

void
FFmpegH264Decoder::OutputDelayedFrames()
{
  while (!mDelayedFrames.IsEmpty()) {
    mCallback->Output(mDelayedFrames.Pop());
  }
}

nsresult
FFmpegH264Decoder::Drain()
{
  
  
  for (int32_t i = 0; i <= mCodecContext.max_b_frames; i++) {
    
    
    nsAutoPtr<MP4Sample> empty(new MP4Sample());

    nsresult rv = Input(empty.forget());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mTaskQueue->Dispatch(
    NS_NewRunnableMethod(this, &FFmpegH264Decoder::OutputDelayedFrames));

  return NS_OK;
}

nsresult
FFmpegH264Decoder::Flush()
{
  nsresult rv = FFmpegDataDecoder::Flush();
  
  mDelayedFrames.Clear();
  return rv;
}

FFmpegH264Decoder::~FFmpegH264Decoder() {
  MOZ_COUNT_DTOR(FFmpegH264Decoder);
  MOZ_ASSERT(mDelayedFrames.IsEmpty());
}

} 
