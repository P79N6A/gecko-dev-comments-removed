










#include "webrtc/modules/video_coding/codecs/vp9/vp9_impl.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include "vpx/vpx_encoder.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vp8dx.h"

#include "webrtc/common.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace webrtc {

VP9Encoder* VP9Encoder::Create() {
  return new VP9EncoderImpl();
}

VP9EncoderImpl::VP9EncoderImpl()
    : encoded_image_(),
      encoded_complete_callback_(NULL),
      inited_(false),
      timestamp_(0),
      picture_id_(0),
      cpu_speed_(3),
      rc_max_intra_target_(0),
      encoder_(NULL),
      config_(NULL),
      raw_(NULL) {
  memset(&codec_, 0, sizeof(codec_));
  uint32_t seed = static_cast<uint32_t>(TickTime::MillisecondTimestamp());
  srand(seed);
}

VP9EncoderImpl::~VP9EncoderImpl() {
  Release();
}

int VP9EncoderImpl::Release() {
  if (encoded_image_._buffer != NULL) {
    delete [] encoded_image_._buffer;
    encoded_image_._buffer = NULL;
  }
  if (encoder_ != NULL) {
    if (vpx_codec_destroy(encoder_)) {
      return WEBRTC_VIDEO_CODEC_MEMORY;
    }
    delete encoder_;
    encoder_ = NULL;
  }
  if (config_ != NULL) {
    delete config_;
    config_ = NULL;
  }
  if (raw_ != NULL) {
    vpx_img_free(raw_);
    raw_ = NULL;
  }
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9EncoderImpl::SetRates(uint32_t new_bitrate_kbit,
                             uint32_t new_framerate) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (encoder_->err) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  if (new_framerate < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  
  if (codec_.maxBitrate > 0 && new_bitrate_kbit > codec_.maxBitrate) {
    new_bitrate_kbit = codec_.maxBitrate;
  }
  config_->rc_target_bitrate = new_bitrate_kbit;
  codec_.maxFramerate = new_framerate;
  
  if (vpx_codec_enc_config_set(encoder_, config_)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9EncoderImpl::InitEncode(const VideoCodec* inst,
                               int number_of_cores,
                               uint32_t ) {
  if (inst == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->maxFramerate < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  
  if (inst->maxBitrate > 0 && inst->startBitrate > inst->maxBitrate) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->width < 1 || inst->height < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (number_of_cores < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  int retVal = Release();
  if (retVal < 0) {
    return retVal;
  }
  if (encoder_ == NULL) {
    encoder_ = new vpx_codec_ctx_t;
  }
  if (config_ == NULL) {
    config_ = new vpx_codec_enc_cfg_t;
  }
  timestamp_ = 0;
  if (&codec_ != inst) {
    codec_ = *inst;
  }
  
  picture_id_ = static_cast<uint16_t>(rand()) & 0x7FFF;
  
  if (encoded_image_._buffer != NULL) {
    delete [] encoded_image_._buffer;
  }
  encoded_image_._size = CalcBufferSize(kI420, codec_.width, codec_.height);
  encoded_image_._buffer = new uint8_t[encoded_image_._size];
  encoded_image_._completeFrame = true;
  
  
  
  raw_ = vpx_img_wrap(NULL, VPX_IMG_FMT_I420, codec_.width, codec_.height,
                      1, NULL);
  
  if (vpx_codec_enc_config_default(vpx_codec_vp9_cx(), config_, 0)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  config_->g_w = codec_.width;
  config_->g_h = codec_.height;
  config_->rc_target_bitrate = inst->startBitrate;  
  config_->g_error_resilient = 1;
  
  config_->g_timebase.num = 1;
  config_->g_timebase.den = 90000;
  config_->g_lag_in_frames = 0;  
  config_->g_threads = 1;
  
  config_->rc_dropframe_thresh = inst->codecSpecific.VP9.frameDroppingOn ?
      30 : 0;
  config_->rc_end_usage = VPX_CBR;
  config_->g_pass = VPX_RC_ONE_PASS;
  config_->rc_min_quantizer = 2;
  config_->rc_max_quantizer = 56;
  config_->rc_undershoot_pct = 50;
  config_->rc_overshoot_pct = 50;
  config_->rc_buf_initial_sz = 500;
  config_->rc_buf_optimal_sz = 600;
  config_->rc_buf_sz = 1000;
  
  rc_max_intra_target_ = MaxIntraTarget(config_->rc_buf_optimal_sz);
  if (inst->codecSpecific.VP9.keyFrameInterval  > 0) {
    config_->kf_mode = VPX_KF_AUTO;
    config_->kf_max_dist = inst->codecSpecific.VP9.keyFrameInterval;
  } else {
    config_->kf_mode = VPX_KF_DISABLED;
  }
  return InitAndSetControlSettings(inst);
}

int VP9EncoderImpl::InitAndSetControlSettings(const VideoCodec* inst) {
  if (vpx_codec_enc_init(encoder_, vpx_codec_vp9_cx(), config_, 0)) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  
  
  cpu_speed_ = 6;
  
  
  vpx_codec_control(encoder_, VP8E_SET_CPUUSED, cpu_speed_);
  vpx_codec_control(encoder_, VP8E_SET_MAX_INTRA_BITRATE_PCT,
                    rc_max_intra_target_);
  vpx_codec_control(encoder_, VP9E_SET_AQ_MODE,
                    inst->codecSpecific.VP9.adaptiveQpMode ? 3 : 0);
  

  
  

  inited_ = true;
  return WEBRTC_VIDEO_CODEC_OK;
}

uint32_t VP9EncoderImpl::MaxIntraTarget(uint32_t optimal_buffer_size) {
  
  
  
  
  
  
  float scale_par = 0.5;
  uint32_t target_pct =
      optimal_buffer_size * scale_par * codec_.maxFramerate / 10;
  
  const uint32_t min_intra_size = 300;
  return (target_pct < min_intra_size) ? min_intra_size: target_pct;
}

int VP9EncoderImpl::Encode(const I420VideoFrame& input_image,
                           const CodecSpecificInfo* codec_specific_info,
                           const std::vector<VideoFrameType>* frame_types) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (input_image.IsZeroSize()) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (encoded_complete_callback_ == NULL) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  VideoFrameType frame_type = kDeltaFrame;
  
  if (frame_types && frame_types->size() > 0) {
    frame_type = (*frame_types)[0];
  }
  
  
  raw_->planes[VPX_PLANE_Y] = const_cast<uint8_t*>(input_image.buffer(kYPlane));
  raw_->planes[VPX_PLANE_U] = const_cast<uint8_t*>(input_image.buffer(kUPlane));
  raw_->planes[VPX_PLANE_V] = const_cast<uint8_t*>(input_image.buffer(kVPlane));
  raw_->stride[VPX_PLANE_Y] = input_image.stride(kYPlane);
  raw_->stride[VPX_PLANE_U] = input_image.stride(kUPlane);
  raw_->stride[VPX_PLANE_V] = input_image.stride(kVPlane);

  int flags = 0;
  bool send_keyframe = (frame_type == kKeyFrame);
  if (send_keyframe) {
    
    flags = VPX_EFLAG_FORCE_KF;
  }
  assert(codec_.maxFramerate > 0);
  uint32_t duration = 90000 / codec_.maxFramerate;
  if (vpx_codec_encode(encoder_, raw_, timestamp_, duration, flags,
                       VPX_DL_REALTIME)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  timestamp_ += duration;
  return GetEncodedPartitions(input_image);
}

void VP9EncoderImpl::PopulateCodecSpecific(CodecSpecificInfo* codec_specific,
                                       const vpx_codec_cx_pkt& pkt,
                                       uint32_t timestamp) {
  assert(codec_specific != NULL);
  codec_specific->codecType = kVideoCodecVP9;
  CodecSpecificInfoVP9 *vp9_info = &(codec_specific->codecSpecific.VP9);
  vp9_info->pictureId = picture_id_;
  vp9_info->keyIdx = kNoKeyIdx;
  vp9_info->nonReference = (pkt.data.frame.flags & VPX_FRAME_IS_DROPPABLE) != 0;
  
  
  
  vp9_info->temporalIdx = kNoTemporalIdx;
  vp9_info->layerSync = false;
  vp9_info->tl0PicIdx = kNoTl0PicIdx;
  picture_id_ = (picture_id_ + 1) & 0x7FFF;
}

int VP9EncoderImpl::GetEncodedPartitions(const I420VideoFrame& input_image) {
  vpx_codec_iter_t iter = NULL;
  encoded_image_._length = 0;
  encoded_image_._frameType = kDeltaFrame;
  RTPFragmentationHeader frag_info;
  
  
  frag_info.VerifyAndAllocateFragmentationHeader(1);
  int part_idx = 0;
  CodecSpecificInfo codec_specific;
  const vpx_codec_cx_pkt_t *pkt = NULL;
  while ((pkt = vpx_codec_get_cx_data(encoder_, &iter)) != NULL) {
    switch (pkt->kind) {
      case VPX_CODEC_CX_FRAME_PKT: {
        memcpy(&encoded_image_._buffer[encoded_image_._length],
               pkt->data.frame.buf,
               pkt->data.frame.sz);
        frag_info.fragmentationOffset[part_idx] = encoded_image_._length;
        frag_info.fragmentationLength[part_idx] =
            static_cast<uint32_t>(pkt->data.frame.sz);
        frag_info.fragmentationPlType[part_idx] = 0;
        frag_info.fragmentationTimeDiff[part_idx] = 0;
        encoded_image_._length += static_cast<uint32_t>(pkt->data.frame.sz);
        assert(encoded_image_._length <= encoded_image_._size);
        break;
      }
      default: {
        break;
      }
    }
    
    if ((pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT) == 0) {
      
      if (pkt->data.frame.flags & VPX_FRAME_IS_KEY) {
        encoded_image_._frameType = kKeyFrame;
      }
      PopulateCodecSpecific(&codec_specific, *pkt, input_image.timestamp());
      break;
    }
  }
  if (encoded_image_._length > 0) {
    TRACE_COUNTER1("webrtc", "EncodedFrameSize", encoded_image_._length);
    encoded_image_._timeStamp = input_image.timestamp();
    encoded_image_.capture_time_ms_ = input_image.render_time_ms();
    encoded_image_._encodedHeight = raw_->d_h;
    encoded_image_._encodedWidth = raw_->d_w;
    encoded_complete_callback_->Encoded(encoded_image_, &codec_specific,
                                      &frag_info);
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9EncoderImpl::SetChannelParameters(uint32_t packet_loss, int rtt) {
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9EncoderImpl::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  encoded_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

VP9Decoder* VP9Decoder::Create() {
  return new VP9DecoderImpl();
}

VP9DecoderImpl::VP9DecoderImpl()
    : decode_complete_callback_(NULL),
      inited_(false),
      decoder_(NULL),
      key_frame_required_(true) {
  memset(&codec_, 0, sizeof(codec_));
}

VP9DecoderImpl::~VP9DecoderImpl() {
  inited_ = true;  
  Release();
}

int VP9DecoderImpl::Reset() {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  InitDecode(&codec_, 1);
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::InitDecode(const VideoCodec* inst, int number_of_cores) {
  if (inst == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  int ret_val = Release();
  if (ret_val < 0) {
    return ret_val;
  }
  if (decoder_ == NULL) {
    decoder_ = new vpx_dec_ctx_t;
  }
  vpx_codec_dec_cfg_t  cfg;
  
  cfg.threads = 1;
  cfg.h = cfg.w = 0;  
  vpx_codec_flags_t flags = 0;
  if (vpx_codec_dec_init(decoder_, vpx_codec_vp9_dx(), &cfg, flags)) {
    return WEBRTC_VIDEO_CODEC_MEMORY;
  }
  if (&codec_ != inst) {
    
    codec_ = *inst;
  }
  inited_ = true;
  
  key_frame_required_ = true;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::Decode(const EncodedImage& input_image,
                           bool missing_frames,
                           const RTPFragmentationHeader* fragmentation,
                           const CodecSpecificInfo* codec_specific_info,
                           int64_t ) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (decode_complete_callback_ == NULL) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  
  if (key_frame_required_) {
    if (input_image._frameType != kKeyFrame)
      return WEBRTC_VIDEO_CODEC_ERROR;
    
    if (input_image._completeFrame) {
      key_frame_required_ = false;
    } else {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }
  vpx_codec_iter_t iter = NULL;
  vpx_image_t* img;
  uint8_t* buffer = input_image._buffer;
  if (input_image._length == 0) {
    buffer = NULL;  
  }
  if (vpx_codec_decode(decoder_,
                       buffer,
                       input_image._length,
                       0,
                       VPX_DL_REALTIME)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  img = vpx_codec_get_frame(decoder_, &iter);
  int ret = ReturnFrame(img, input_image._timeStamp);
  if (ret != 0) {
    return ret;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::ReturnFrame(const vpx_image_t* img, uint32_t timestamp) {
  if (img == NULL) {
    
    return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
  }
  int half_height = (img->d_h + 1) / 2;
  int size_y = img->stride[VPX_PLANE_Y] * img->d_h;
  int size_u = img->stride[VPX_PLANE_U] * half_height;
  int size_v = img->stride[VPX_PLANE_V] * half_height;
  decoded_image_.CreateFrame(size_y, img->planes[VPX_PLANE_Y],
                             size_u, img->planes[VPX_PLANE_U],
                             size_v, img->planes[VPX_PLANE_V],
                             img->d_w, img->d_h,
                             img->stride[VPX_PLANE_Y],
                             img->stride[VPX_PLANE_U],
                             img->stride[VPX_PLANE_V]);
  decoded_image_.set_timestamp(timestamp);
  int ret = decode_complete_callback_->Decoded(decoded_image_);
  if (ret != 0)
    return ret;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::RegisterDecodeCompleteCallback(
    DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::Release() {
  if (decoder_ != NULL) {
    if (vpx_codec_destroy(decoder_)) {
      return WEBRTC_VIDEO_CODEC_MEMORY;
    }
    delete decoder_;
    decoder_ = NULL;
  }
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}
}  
