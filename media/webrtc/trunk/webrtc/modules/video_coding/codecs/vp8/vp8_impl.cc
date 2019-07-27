












#include "webrtc/modules/video_coding/codecs/vp8/vp8_impl.h"

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
#include "webrtc/modules/video_coding/codecs/vp8/temporal_layers.h"
#include "webrtc/modules/video_coding/codecs/vp8/reference_picture_selection.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

enum { kVp8ErrorPropagationTh = 30 };

namespace webrtc {

VP8Encoder* VP8Encoder::Create() {
  return new VP8EncoderImpl();
}

VP8EncoderImpl::VP8EncoderImpl()
    : encoded_image_(),
      encoded_complete_callback_(NULL),
      inited_(false),
      timestamp_(0),
      picture_id_(0),
      feedback_mode_(false),
      cpu_speed_(-6),  
      rc_max_intra_target_(0),
      token_partitions_(VP8_ONE_TOKENPARTITION),
      rps_(new ReferencePictureSelection),
      temporal_layers_(NULL),
      encoder_(NULL),
      config_(NULL),
      raw_(NULL) {
  memset(&codec_, 0, sizeof(codec_));
  uint32_t seed = static_cast<uint32_t>(TickTime::MillisecondTimestamp());
  srand(seed);
}

VP8EncoderImpl::~VP8EncoderImpl() {
  Release();
  delete rps_;
}

int VP8EncoderImpl::Release() {
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
  delete temporal_layers_;
  temporal_layers_ = NULL;
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8EncoderImpl::SetRates(uint32_t new_bitrate_kbit,
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
  temporal_layers_->ConfigureBitrates(new_bitrate_kbit, codec_.maxBitrate,
                                      new_framerate, config_);
  codec_.maxFramerate = new_framerate;

  
  if (vpx_codec_enc_config_set(encoder_, config_)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8EncoderImpl::InitEncode(const VideoCodec* inst,
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
  feedback_mode_ = inst->codecSpecific.VP8.feedbackModeOn;

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

  
  Config default_options;
  const Config& options =
      inst->extra_options ? *inst->extra_options : default_options;

  int num_temporal_layers = inst->codecSpecific.VP8.numberOfTemporalLayers > 1 ?
      inst->codecSpecific.VP8.numberOfTemporalLayers : 1;
  assert(temporal_layers_ == NULL);
  temporal_layers_ = options.Get<TemporalLayers::Factory>()
                         .Create(num_temporal_layers, rand());
  
  picture_id_ = static_cast<uint16_t>(rand()) & 0x7FFF;

  
  if (encoded_image_._buffer != NULL) {
    delete [] encoded_image_._buffer;
  }
  
  encoded_image_._size = CalcBufferSize(kI420, codec_.width, codec_.height)
                         + 100;
  encoded_image_._buffer = new uint8_t[encoded_image_._size];
  encoded_image_._completeFrame = true;

  
  
  
  raw_ = vpx_img_wrap(NULL, VPX_IMG_FMT_I420, codec_.width, codec_.height,
                      1, NULL);
  
  if (vpx_codec_enc_config_default(vpx_codec_vp8_cx(), config_, 0)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  config_->g_w = codec_.width;
  config_->g_h = codec_.height;
  config_->rc_target_bitrate = inst->startBitrate;  
  temporal_layers_->ConfigureBitrates(inst->startBitrate, inst->maxBitrate,
                                      inst->maxFramerate, config_);
  
  config_->g_timebase.num = 1;
  config_->g_timebase.den = 90000;

  
  switch (inst->codecSpecific.VP8.resilience) {
    case kResilienceOff:
      config_->g_error_resilient = 0;
      if (num_temporal_layers > 1) {
        
        config_->g_error_resilient = 1;
      }
      break;
    case kResilientStream:
      config_->g_error_resilient = 1;  
      
      
      break;
    case kResilientFrames:
#ifdef INDEPENDENT_PARTITIONS
      config_->g_error_resilient = VPX_ERROR_RESILIENT_DEFAULT |
      VPX_ERROR_RESILIENT_PARTITIONS;
      break;
#else
      return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;  
#endif
  }
  config_->g_lag_in_frames = 0;  

  if (codec_.width * codec_.height > 1280 * 960 && number_of_cores >= 6) {
    config_->g_threads = 3;  
  } else if (codec_.width * codec_.height > 640 * 480 && number_of_cores >= 3) {
    config_->g_threads = 2;  
  } else {
    config_->g_threads = 1;  
  }

  
  config_->rc_dropframe_thresh = inst->codecSpecific.VP8.frameDroppingOn ?
      30 : 0;
  config_->rc_end_usage = VPX_CBR;
  config_->g_pass = VPX_RC_ONE_PASS;
  config_->rc_resize_allowed = inst->codecSpecific.VP8.automaticResizeOn ?
      1 : 0;
  config_->rc_min_quantizer = 2;
  config_->rc_max_quantizer = inst->qpMax;
  config_->rc_undershoot_pct = 100;
  config_->rc_overshoot_pct = 15;
  config_->rc_buf_initial_sz = 500;
  config_->rc_buf_optimal_sz = 600;
  config_->rc_buf_sz = 1000;
  
  rc_max_intra_target_ = MaxIntraTarget(config_->rc_buf_optimal_sz);

  if (feedback_mode_) {
    
    
    config_->kf_mode = VPX_KF_DISABLED;
  } else if (inst->codecSpecific.VP8.keyFrameInterval  > 0) {
    config_->kf_mode = VPX_KF_AUTO;
    config_->kf_max_dist = inst->codecSpecific.VP8.keyFrameInterval;
  } else {
    config_->kf_mode = VPX_KF_DISABLED;
  }
  switch (inst->codecSpecific.VP8.complexity) {
    case kComplexityHigh:
      cpu_speed_ = -5;
      break;
    case kComplexityHigher:
      cpu_speed_ = -4;
      break;
    case kComplexityMax:
      cpu_speed_ = -3;
      break;
    default:
      cpu_speed_ = -6;
      break;
  }
#if defined(WEBRTC_ARCH_ARM)
  
  
  cpu_speed_ = -12;
#endif
  rps_->Init();
  return InitAndSetControlSettings(inst);
}

int VP8EncoderImpl::InitAndSetControlSettings(const VideoCodec* inst) {
  vpx_codec_flags_t flags = 0;
  
  
  
  flags |= VPX_CODEC_USE_OUTPUT_PARTITION;
  if (vpx_codec_enc_init(encoder_, vpx_codec_vp8_cx(), config_, flags)) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  vpx_codec_control(encoder_, VP8E_SET_STATIC_THRESHOLD, 1);
  vpx_codec_control(encoder_, VP8E_SET_CPUUSED, cpu_speed_);
  vpx_codec_control(encoder_, VP8E_SET_TOKEN_PARTITIONS,
                    static_cast<vp8e_token_partitions>(token_partitions_));
#if !defined(WEBRTC_ARCH_ARM)
  
  vpx_codec_control(encoder_, VP8E_SET_NOISE_SENSITIVITY,
                    inst->codecSpecific.VP8.denoisingOn ? 1 : 0);
#endif
  vpx_codec_control(encoder_, VP8E_SET_MAX_INTRA_BITRATE_PCT,
                    rc_max_intra_target_);
  inited_ = true;
  return WEBRTC_VIDEO_CODEC_OK;
}

uint32_t VP8EncoderImpl::MaxIntraTarget(uint32_t optimalBuffersize) {
  
  
  
  
  
  

  float scalePar = 0.5;
  uint32_t targetPct = optimalBuffersize * scalePar * codec_.maxFramerate / 10;

  
  const uint32_t minIntraTh = 300;
  return (targetPct < minIntraTh) ? minIntraTh: targetPct;
}

int VP8EncoderImpl::Encode(const I420VideoFrame& input_image,
                           const CodecSpecificInfo* codec_specific_info,
                           const std::vector<VideoFrameType>* frame_types) {
  TRACE_EVENT1("webrtc", "VP8::Encode", "timestamp", input_image.timestamp());

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

  
  if (input_image.width() != codec_.width ||
      input_image.height() != codec_.height) {
    int ret = UpdateCodecFrameSize(input_image);
    if (ret < 0) {
      return ret;
    }
#ifndef LIBVPX_ENCODER_CONFIG_ON_RESIZE 
    frame_type = kKeyFrame;
#endif
  }

  
  
  raw_->planes[VPX_PLANE_Y] = const_cast<uint8_t*>(input_image.buffer(kYPlane));
  raw_->planes[VPX_PLANE_U] = const_cast<uint8_t*>(input_image.buffer(kUPlane));
  raw_->planes[VPX_PLANE_V] = const_cast<uint8_t*>(input_image.buffer(kVPlane));
  
  raw_->stride[VPX_PLANE_Y] = input_image.stride(kYPlane);
  raw_->stride[VPX_PLANE_U] = input_image.stride(kUPlane);
  raw_->stride[VPX_PLANE_V] = input_image.stride(kVPlane);

  int flags = temporal_layers_->EncodeFlags(input_image.timestamp());

  bool send_keyframe = (frame_type == kKeyFrame);
  if (send_keyframe) {
    
    
    flags = VPX_EFLAG_FORCE_KF;
  } else if (feedback_mode_ && codec_specific_info) {
    
    bool sendRefresh = false;
    if (codec_specific_info->codecType == kVideoCodecVP8) {
      if (codec_specific_info->codecSpecific.VP8.hasReceivedRPSI) {
        rps_->ReceivedRPSI(
            codec_specific_info->codecSpecific.VP8.pictureIdRPSI);
      }
      if (codec_specific_info->codecSpecific.VP8.hasReceivedSLI) {
        sendRefresh = rps_->ReceivedSLI(input_image.timestamp());
      }
    }
    flags = rps_->EncodeFlags(picture_id_, sendRefresh,
                              input_image.timestamp());
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

int VP8EncoderImpl::UpdateCodecFrameSize(const I420VideoFrame& input_image) {
  codec_.width = input_image.width();
  codec_.height = input_image.height();
  raw_->w = codec_.width;
  raw_->h = codec_.height;
  raw_->d_w = codec_.width;
  raw_->d_h = codec_.height;

  raw_->stride[VPX_PLANE_Y] = input_image.stride(kYPlane);
  raw_->stride[VPX_PLANE_U] = input_image.stride(kUPlane);
  raw_->stride[VPX_PLANE_V] = input_image.stride(kVPlane);
  vpx_img_set_rect(raw_, 0, 0, codec_.width, codec_.height);

  
  
  config_->g_w = codec_.width;
  config_->g_h = codec_.height;
#ifndef LIBVPX_ENCODER_CONFIG_ON_RESIZE
  
  
  
  vpx_codec_flags_t flags = VPX_CODEC_USE_OUTPUT_PARTITION;
  if (vpx_codec_enc_init(encoder_, vpx_codec_vp8_cx(), config_, flags)) {
#else
  if (vpx_codec_enc_config_set(encoder_, config_)) {
#endif
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

void VP8EncoderImpl::PopulateCodecSpecific(CodecSpecificInfo* codec_specific,
                                       const vpx_codec_cx_pkt& pkt,
                                       uint32_t timestamp) {
  assert(codec_specific != NULL);
  codec_specific->codecType = kVideoCodecVP8;
  CodecSpecificInfoVP8 *vp8Info = &(codec_specific->codecSpecific.VP8);
  vp8Info->pictureId = picture_id_;
  vp8Info->simulcastIdx = 0;
  vp8Info->keyIdx = kNoKeyIdx;  
  vp8Info->nonReference = (pkt.data.frame.flags & VPX_FRAME_IS_DROPPABLE) != 0;
  temporal_layers_->PopulateCodecSpecific(
      (pkt.data.frame.flags & VPX_FRAME_IS_KEY) ? true : false, vp8Info,
          timestamp);
  picture_id_ = (picture_id_ + 1) & 0x7FFF;  
}

int VP8EncoderImpl::GetEncodedPartitions(const I420VideoFrame& input_image) {
  vpx_codec_iter_t iter = NULL;
  int part_idx = 0;
  encoded_image_._length = 0;
  encoded_image_._frameType = kDeltaFrame;
  RTPFragmentationHeader frag_info;
  frag_info.VerifyAndAllocateFragmentationHeader((1 << token_partitions_) + 1);
  CodecSpecificInfo codec_specific;

  const vpx_codec_cx_pkt_t *pkt = NULL;
  while ((pkt = vpx_codec_get_cx_data(encoder_, &iter)) != NULL) {
    switch (pkt->kind) {
      case VPX_CODEC_CX_FRAME_PKT: {
        memcpy(&encoded_image_._buffer[encoded_image_._length],
               pkt->data.frame.buf,
               pkt->data.frame.sz);
        frag_info.fragmentationOffset[part_idx] = encoded_image_._length;
        frag_info.fragmentationLength[part_idx] =  pkt->data.frame.sz;
        frag_info.fragmentationPlType[part_idx] = 0;  
        frag_info.fragmentationTimeDiff[part_idx] = 0;
        encoded_image_._length += pkt->data.frame.sz;
        assert(encoded_image_._length <= encoded_image_._size);
        ++part_idx;
        break;
      }
      default: {
        break;
      }
    }
    
    if ((pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT) == 0) {
      
      if (pkt->data.frame.flags & VPX_FRAME_IS_KEY) {
          encoded_image_._frameType = kKeyFrame;
          rps_->EncodedKeyFrame(picture_id_);
      }
      PopulateCodecSpecific(&codec_specific, *pkt, input_image.timestamp());
      break;
    }
  }
  if (encoded_image_._length > 0) {
    TRACE_COUNTER1("webrtc", "EncodedFrameSize", encoded_image_._length);
    encoded_image_._timeStamp = input_image.timestamp();
    encoded_image_.capture_time_ms_ = input_image.render_time_ms();
    encoded_image_._encodedHeight = raw_->h;
    encoded_image_._encodedWidth = raw_->w;
    encoded_complete_callback_->Encoded(encoded_image_, &codec_specific,
                                      &frag_info);
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8EncoderImpl::SetChannelParameters(uint32_t , int rtt) {
  rps_->SetRtt(rtt);
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8EncoderImpl::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  encoded_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

VP8Decoder* VP8Decoder::Create() {
  return new VP8DecoderImpl();
}

VP8DecoderImpl::VP8DecoderImpl()
    : decode_complete_callback_(NULL),
      inited_(false),
      feedback_mode_(false),
      decoder_(NULL),
      last_keyframe_(),
      image_format_(VPX_IMG_FMT_NONE),
      ref_frame_(NULL),
      propagation_cnt_(-1),
      mfqe_enabled_(false),
      key_frame_required_(true) {
  memset(&codec_, 0, sizeof(codec_));
}

VP8DecoderImpl::~VP8DecoderImpl() {
  inited_ = true;  
  Release();
}

int VP8DecoderImpl::Reset() {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  InitDecode(&codec_, 1);
  propagation_cnt_ = -1;
  mfqe_enabled_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8DecoderImpl::InitDecode(const VideoCodec* inst, int number_of_cores) {
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
  if (inst->codecType == kVideoCodecVP8) {
    feedback_mode_ = inst->codecSpecific.VP8.feedbackModeOn;
  }
  vpx_codec_dec_cfg_t  cfg;
  
  cfg.threads = 1;
  cfg.h = cfg.w = 0;  

  vpx_codec_flags_t flags = 0;
#ifndef WEBRTC_ARCH_ARM
  flags = VPX_CODEC_USE_POSTPROC;
  if (inst->codecSpecific.VP8.errorConcealmentOn) {
    flags |= VPX_CODEC_USE_ERROR_CONCEALMENT;
  }
#ifdef INDEPENDENT_PARTITIONS
  flags |= VPX_CODEC_USE_INPUT_PARTITION;
#endif
#endif

  if (vpx_codec_dec_init(decoder_, vpx_codec_vp8_dx(), &cfg, flags)) {
    return WEBRTC_VIDEO_CODEC_MEMORY;
  }

#ifndef WEBRTC_ARCH_ARM
  vp8_postproc_cfg_t  ppcfg;
  ppcfg.post_proc_flag = VP8_DEMACROBLOCK | VP8_DEBLOCK;
  
  ppcfg.deblocking_level = 3;
  vpx_codec_control(decoder_, VP8_SET_POSTPROC, &ppcfg);
#endif

  if (&codec_ != inst) {
    
    codec_ = *inst;
  }

  propagation_cnt_ = -1;

  inited_ = true;

  
  key_frame_required_ = true;

  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8DecoderImpl::Decode(const EncodedImage& input_image,
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
  if (input_image._buffer == NULL && input_image._length > 0) {
    
    if (propagation_cnt_ > 0)
      propagation_cnt_ = 0;
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

#ifdef INDEPENDENT_PARTITIONS
  if (fragmentation == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
#endif

#ifndef WEBRTC_ARCH_ARM
  if (!mfqe_enabled_ && codec_specific_info &&
      codec_specific_info->codecSpecific.VP8.temporalIdx > 0) {
    
    
    
    mfqe_enabled_ = true;
    vp8_postproc_cfg_t  ppcfg;
    ppcfg.post_proc_flag = VP8_MFQE | VP8_DEMACROBLOCK | VP8_DEBLOCK;
    ppcfg.deblocking_level = 3;
    vpx_codec_control(decoder_, VP8_SET_POSTPROC, &ppcfg);
  }
#endif


  
  if (key_frame_required_) {
    if (input_image._frameType != kKeyFrame)
      return WEBRTC_VIDEO_CODEC_ERROR;
    
    if (input_image._completeFrame) {
      key_frame_required_ = false;
    } else {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }
  
  
  
  if (!feedback_mode_) {
    if (input_image._frameType == kKeyFrame && input_image._completeFrame)
      propagation_cnt_ = -1;
    
    else if ((!input_image._completeFrame || missing_frames) &&
        propagation_cnt_ == -1)
      propagation_cnt_ = 0;
    if (propagation_cnt_ >= 0)
      propagation_cnt_++;
  }

  vpx_codec_iter_t iter = NULL;
  vpx_image_t* img;
  int ret;

  
  if (missing_frames) {
    
    if (vpx_codec_decode(decoder_, NULL, 0, 0, VPX_DL_REALTIME)) {
      
      if (propagation_cnt_ > 0)
        propagation_cnt_ = 0;
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    
    vpx_codec_get_frame(decoder_, &iter);
    iter = NULL;
  }

#ifdef INDEPENDENT_PARTITIONS
  if (DecodePartitions(inputImage, fragmentation)) {
    
    if (propagation_cnt_ > 0) {
      propagation_cnt_ = 0;
    }
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
#else
  uint8_t* buffer = input_image._buffer;
  if (input_image._length == 0) {
    buffer = NULL;  
  }
  if (vpx_codec_decode(decoder_,
                       buffer,
                       input_image._length,
                       0,
                       VPX_DL_REALTIME)) {
    
    if (propagation_cnt_ > 0)
      propagation_cnt_ = 0;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
#endif

  
  if (input_image._frameType == kKeyFrame && input_image._buffer != NULL) {
    const uint32_t bytes_to_copy = input_image._length;
    if (last_keyframe_._size < bytes_to_copy) {
      delete [] last_keyframe_._buffer;
      last_keyframe_._buffer = NULL;
      last_keyframe_._size = 0;
    }

    uint8_t* temp_buffer = last_keyframe_._buffer;  
    uint32_t temp_size = last_keyframe_._size;  
    last_keyframe_ = input_image;  
    last_keyframe_._buffer = temp_buffer;  
    last_keyframe_._size = temp_size;  
    if (!last_keyframe_._buffer) {
      
      last_keyframe_._size = bytes_to_copy;
      last_keyframe_._buffer = new uint8_t[last_keyframe_._size];
    }
    
    memcpy(last_keyframe_._buffer, input_image._buffer, bytes_to_copy);
    last_keyframe_._length = bytes_to_copy;
  }

  img = vpx_codec_get_frame(decoder_, &iter);
  ret = ReturnFrame(img, input_image._timeStamp);
  if (ret != 0) {
    
    if (ret < 0 && propagation_cnt_ > 0)
      propagation_cnt_ = 0;
    return ret;
  }
  if (feedback_mode_) {
    
    
    
    if (input_image._frameType == kKeyFrame && !input_image._completeFrame)
      return WEBRTC_VIDEO_CODEC_ERROR;

    
    
    
    int reference_updates = 0;
    if (vpx_codec_control(decoder_, VP8D_GET_LAST_REF_UPDATES,
                          &reference_updates)) {
      
      if (propagation_cnt_ > 0)
        propagation_cnt_ = 0;
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    int corrupted = 0;
    if (vpx_codec_control(decoder_, VP8D_GET_FRAME_CORRUPTED, &corrupted)) {
      
      if (propagation_cnt_ > 0)
        propagation_cnt_ = 0;
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    int16_t picture_id = -1;
    if (codec_specific_info) {
      picture_id = codec_specific_info->codecSpecific.VP8.pictureId;
    }
    if (picture_id > -1) {
      if (((reference_updates & VP8_GOLD_FRAME) ||
          (reference_updates & VP8_ALTR_FRAME)) && !corrupted) {
        decode_complete_callback_->ReceivedDecodedReferenceFrame(picture_id);
      }
      decode_complete_callback_->ReceivedDecodedFrame(picture_id);
    }
    if (corrupted) {
      
      return WEBRTC_VIDEO_CODEC_REQUEST_SLI;
    }
  }
  
  if (propagation_cnt_ > kVp8ErrorPropagationTh) {
    
    propagation_cnt_ = 0;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8DecoderImpl::DecodePartitions(
    const EncodedImage& input_image,
    const RTPFragmentationHeader* fragmentation) {
  for (int i = 0; i < fragmentation->fragmentationVectorSize; ++i) {
    const uint8_t* partition = input_image._buffer +
        fragmentation->fragmentationOffset[i];
    const uint32_t partition_length =
        fragmentation->fragmentationLength[i];
    if (vpx_codec_decode(decoder_,
                         partition,
                         partition_length,
                         0,
                         VPX_DL_REALTIME)) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }
  
  
  if (vpx_codec_decode(decoder_, NULL, 0, 0, VPX_DL_REALTIME))
    return WEBRTC_VIDEO_CODEC_ERROR;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8DecoderImpl::ReturnFrame(const vpx_image_t* img, uint32_t timestamp) {
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

  
  image_format_ = img->fmt;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8DecoderImpl::RegisterDecodeCompleteCallback(
    DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP8DecoderImpl::Release() {
  if (last_keyframe_._buffer != NULL) {
    delete [] last_keyframe_._buffer;
    last_keyframe_._buffer = NULL;
  }
  if (decoder_ != NULL) {
    if (vpx_codec_destroy(decoder_)) {
      return WEBRTC_VIDEO_CODEC_MEMORY;
    }
    delete decoder_;
    decoder_ = NULL;
  }
  if (ref_frame_ != NULL) {
    vpx_img_free(&ref_frame_->img);
    delete ref_frame_;
    ref_frame_ = NULL;
  }
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

VideoDecoder* VP8DecoderImpl::Copy() {
  
  if (!inited_) {
    
    assert(false);
    return NULL;
  }
  if (decoded_image_.IsZeroSize()) {
    
    return NULL;
  }
  if (last_keyframe_._buffer == NULL) {
    
    return NULL;
  }
  
  VP8DecoderImpl *copy = new VP8DecoderImpl;

  
  if (copy->InitDecode(&codec_, 1) != WEBRTC_VIDEO_CODEC_OK) {
    delete copy;
    return NULL;
  }
  
  if (vpx_codec_decode(copy->decoder_, last_keyframe_._buffer,
                       last_keyframe_._length, NULL, VPX_DL_REALTIME)) {
    delete copy;
    return NULL;
  }
  
  assert(decoded_image_.width() > 0);
  assert(decoded_image_.height() > 0);
  assert(image_format_ > VPX_IMG_FMT_NONE);
  
  if (ref_frame_ &&
      (decoded_image_.width() != static_cast<int>(ref_frame_->img.d_w) ||
          decoded_image_.height() != static_cast<int>(ref_frame_->img.d_h) ||
          image_format_ != ref_frame_->img.fmt)) {
    vpx_img_free(&ref_frame_->img);
    delete ref_frame_;
    ref_frame_ = NULL;
  }


  if (!ref_frame_) {
    ref_frame_ = new vpx_ref_frame_t;

    unsigned int align = 16;
    if (!vpx_img_alloc(&ref_frame_->img,
                       static_cast<vpx_img_fmt_t>(image_format_),
                       decoded_image_.width(), decoded_image_.height(),
                       align)) {
      assert(false);
      delete copy;
      return NULL;
    }
  }
  const vpx_ref_frame_type_t type_vec[] = { VP8_LAST_FRAME, VP8_GOLD_FRAME,
      VP8_ALTR_FRAME };
  for (uint32_t ix = 0;
      ix < sizeof(type_vec) / sizeof(vpx_ref_frame_type_t); ++ix) {
    ref_frame_->frame_type = type_vec[ix];
    if (CopyReference(copy) < 0) {
      delete copy;
      return NULL;
    }
  }
  
  copy->feedback_mode_ = feedback_mode_;
  copy->image_format_ = image_format_;
  copy->last_keyframe_ = last_keyframe_;  
  
  copy->last_keyframe_._buffer = new uint8_t[last_keyframe_._size];
  memcpy(copy->last_keyframe_._buffer, last_keyframe_._buffer,
         last_keyframe_._length);

  return static_cast<VideoDecoder*>(copy);
}

int VP8DecoderImpl::CopyReference(VP8Decoder* copyTo) {
  
  
  if (vpx_codec_control(decoder_, VP8_COPY_REFERENCE, ref_frame_)
      != VPX_CODEC_OK) {
    return -1;
  }
  if (vpx_codec_control(static_cast<VP8DecoderImpl*>(copyTo)->decoder_,
                        VP8_SET_REFERENCE, ref_frame_) != VPX_CODEC_OK) {
    return -1;
  }
  return 0;
}

}  
