




#include "SoftwareWebMVideoDecoder.h"
#include "AbstractMediaDecoder.h"
#include "gfx2DGlue.h"
#include "MediaDecoderStateMachine.h"
#include "MediaResource.h"
#include "nsError.h"
#include "OggReader.h"
#include "TimeUnits.h"
#include "VorbisUtils.h"
#include "WebMBufferedParser.h"
#include "NesteggPacketHolder.h"

#include <algorithm>

#define VPX_DONT_DEFINE_STDINT_TYPES
#include "vpx/vp8dx.h"
#include "vpx/vpx_decoder.h"

namespace mozilla {

using namespace gfx;
using namespace layers;

SoftwareWebMVideoDecoder::SoftwareWebMVideoDecoder(WebMReader* aReader)
  : WebMVideoDecoder(),
    mReader(aReader)
{
  MOZ_COUNT_CTOR(SoftwareWebMVideoDecoder);
  PodZero(&mVPX);
}

SoftwareWebMVideoDecoder::~SoftwareWebMVideoDecoder()
{
  MOZ_COUNT_DTOR(SoftwareWebMVideoDecoder);
}

void
SoftwareWebMVideoDecoder::Shutdown()
{
  vpx_codec_destroy(&mVPX);
  mReader = nullptr;
}


WebMVideoDecoder*
SoftwareWebMVideoDecoder::Create(WebMReader* aReader)
{
  return new SoftwareWebMVideoDecoder(aReader);
}

nsresult
SoftwareWebMVideoDecoder::Init(unsigned int aWidth, unsigned int aHeight)
{
  vpx_codec_iface_t* dx = nullptr;
  switch(mReader->GetVideoCodec()) {
    case NESTEGG_CODEC_VP8:
      dx = vpx_codec_vp8_dx();
      break;
    case NESTEGG_CODEC_VP9:
      dx = vpx_codec_vp9_dx();
      break;
  }
  if (!dx || vpx_codec_dec_init(&mVPX, dx, nullptr, 0)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

bool
SoftwareWebMVideoDecoder::DecodeVideoFrame(bool &aKeyframeSkip,
                                           int64_t aTimeThreshold)
{
  MOZ_ASSERT(mReader->OnTaskQueue());

  
  
  AbstractMediaDecoder::AutoNotifyDecoded a(mReader->GetDecoder());

  nsRefPtr<NesteggPacketHolder> holder(mReader->NextPacket(WebMReader::VIDEO));
  if (!holder) {
    return false;
  }

  nestegg_packet* packet = holder->Packet();
  unsigned int track = 0;
  int r = nestegg_packet_track(packet, &track);
  if (r == -1) {
    return false;
  }

  unsigned int count = 0;
  r = nestegg_packet_count(packet, &count);
  if (r == -1) {
    return false;
  }

  if (count > 1) {
    NS_WARNING("Packet contains more than one video frame");
    return false;
  }

  int64_t tstamp = holder->Timestamp();

  
  
  
  
  int64_t next_tstamp = 0;
  nsRefPtr<NesteggPacketHolder> next_holder(mReader->NextPacket(WebMReader::VIDEO));
  if (next_holder) {
    next_tstamp = next_holder->Timestamp();
    mReader->PushVideoPacket(next_holder.forget());
  } else {
    next_tstamp = tstamp;
    next_tstamp += tstamp - mReader->GetLastVideoFrameTime();
  }
  mReader->SetLastVideoFrameTime(tstamp);

  unsigned char* data;
  size_t length;
  r = nestegg_packet_data(packet, 0, &data, &length);
  if (r == -1) {
    return false;
  }

  vpx_codec_stream_info_t si;
  PodZero(&si);
  si.sz = sizeof(si);
  if (mReader->GetVideoCodec() == NESTEGG_CODEC_VP8) {
    vpx_codec_peek_stream_info(vpx_codec_vp8_dx(), data, length, &si);
  } else if (mReader->GetVideoCodec() == NESTEGG_CODEC_VP9) {
    vpx_codec_peek_stream_info(vpx_codec_vp9_dx(), data, length, &si);
  }
  if (aKeyframeSkip && (!si.is_kf || tstamp < aTimeThreshold)) {
    
    a.mParsed++;
    a.mDropped++;
    return true;
  }

  if (aKeyframeSkip && si.is_kf) {
    aKeyframeSkip = false;
  }

  if (vpx_codec_decode(&mVPX, data, length, nullptr, 0)) {
    return false;
  }

  
  
  
  if (tstamp < aTimeThreshold) {
    a.mParsed++;
    a.mDropped++;
    return true;
  }

  vpx_codec_iter_t  iter = nullptr;
  vpx_image_t      *img;

  while ((img = vpx_codec_get_frame(&mVPX, &iter))) {
    NS_ASSERTION(img->fmt == VPX_IMG_FMT_I420, "WebM image format not I420");

    
    VideoData::YCbCrBuffer b;
    b.mPlanes[0].mData = img->planes[0];
    b.mPlanes[0].mStride = img->stride[0];
    b.mPlanes[0].mHeight = img->d_h;
    b.mPlanes[0].mWidth = img->d_w;
    b.mPlanes[0].mOffset = b.mPlanes[0].mSkip = 0;

    b.mPlanes[1].mData = img->planes[1];
    b.mPlanes[1].mStride = img->stride[1];
    b.mPlanes[1].mHeight = (img->d_h + 1) >> img->y_chroma_shift;
    b.mPlanes[1].mWidth = (img->d_w + 1) >> img->x_chroma_shift;
    b.mPlanes[1].mOffset = b.mPlanes[1].mSkip = 0;

    b.mPlanes[2].mData = img->planes[2];
    b.mPlanes[2].mStride = img->stride[2];
    b.mPlanes[2].mHeight = (img->d_h + 1) >> img->y_chroma_shift;
    b.mPlanes[2].mWidth = (img->d_w + 1) >> img->x_chroma_shift;
    b.mPlanes[2].mOffset = b.mPlanes[2].mSkip = 0;

    nsIntRect pictureRect = mReader->GetPicture();
    IntRect picture = pictureRect;
    nsIntSize initFrame = mReader->GetInitialFrame();
    if (img->d_w != static_cast<uint32_t>(initFrame.width) ||
        img->d_h != static_cast<uint32_t>(initFrame.height)) {
      
      
      
      
      picture.x = (pictureRect.x * img->d_w) / initFrame.width;
      picture.y = (pictureRect.y * img->d_h) / initFrame.height;
      picture.width = (img->d_w * pictureRect.width) / initFrame.width;
      picture.height = (img->d_h * pictureRect.height) / initFrame.height;
    }

    VideoInfo videoInfo = mReader->GetMediaInfo().mVideo;
    nsRefPtr<VideoData> v = VideoData::Create(videoInfo,
                                              mReader->GetDecoder()->GetImageContainer(),
                                              holder->Offset(),
                                              tstamp,
                                              next_tstamp - tstamp,
                                              b,
                                              si.is_kf,
                                              -1,
                                              picture);
    if (!v) {
      return false;
    }
    a.mParsed++;
    a.mDecoded++;
    NS_ASSERTION(a.mDecoded <= a.mParsed,
                 "Expect only 1 frame per chunk per packet in WebM...");
    mReader->VideoQueue().Push(v);
  }

  return true;
}

} 
