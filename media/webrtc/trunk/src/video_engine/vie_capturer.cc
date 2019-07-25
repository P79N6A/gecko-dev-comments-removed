









#include "video_engine/vie_capturer.h"

#include "modules/interface/module_common_types.h"
#include "modules/utility/interface/process_thread.h"
#include "modules/video_capture/main/interface/video_capture_factory.h"
#include "modules/video_processing/main/interface/video_processing.h"
#include "modules/video_render/main/interface/video_render_defines.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/event_wrapper.h"
#include "system_wrappers/interface/thread_wrapper.h"
#include "system_wrappers/interface/trace.h"
#include "video_engine/include/vie_image_process.h"
#include "video_engine/vie_defines.h"
#include "video_engine/vie_encoder.h"

namespace webrtc {

const int kThreadWaitTimeMs = 100;
const int kMaxDeliverWaitTime = 500;

ViECapturer::ViECapturer(int capture_id,
                         int engine_id,
                         ProcessThread& module_process_thread)
    : ViEFrameProviderBase(capture_id, engine_id),
      capture_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      deliver_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      capture_module_(NULL),
      external_capture_module_(NULL),
      module_process_thread_(module_process_thread),
      capture_id_(capture_id),
      capture_thread_(*ThreadWrapper::CreateThread(ViECaptureThreadFunction,
                                                   this, kHighPriority,
                                                   "ViECaptureThread")),
      capture_event_(*EventWrapper::Create()),
      deliver_event_(*EventWrapper::Create()),
      effect_filter_(NULL),
      image_proc_module_(NULL),
      image_proc_module_ref_counter_(0),
      deflicker_frame_stats_(NULL),
      brightness_frame_stats_(NULL),
      current_brightness_level_(Normal),
      reported_brightness_level_(Normal),
      denoising_enabled_(false),
      observer_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      observer_(NULL),
      encoding_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      capture_encoder_(NULL),
      encode_complete_callback_(NULL),
      vie_encoder_(NULL),
      vcm_(NULL),
      decoder_initialized_(false) {
  WEBRTC_TRACE(kTraceMemory, kTraceVideo, ViEId(engine_id, capture_id),
               "ViECapturer::ViECapturer(capture_id: %d, engine_id: %d)",
               capture_id, engine_id);
  unsigned int t_id = 0;
  if (capture_thread_.Start(t_id)) {
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id, capture_id),
                 "%s: thread started: %u", __FUNCTION__, t_id);
  } else {
    assert(false);
  }
}

ViECapturer::~ViECapturer() {
  WEBRTC_TRACE(kTraceMemory, kTraceVideo, ViEId(engine_id_, capture_id_),
               "ViECapturer::~ViECapturer() - capture_id: %d, engine_id: %d",
               capture_id_, engine_id_);

  
  deliver_cs_->Enter();
  capture_cs_->Enter();
  capture_thread_.SetNotAlive();
  capture_event_.Set();
  capture_cs_->Leave();
  deliver_cs_->Leave();

  provider_cs_->Enter();
  if (vie_encoder_) {
    vie_encoder_->DeRegisterExternalEncoder(codec_.plType);
  }
  provider_cs_->Leave();

  
  if (capture_module_) {
    module_process_thread_.DeRegisterModule(capture_module_);
    capture_module_->DeRegisterCaptureDataCallback();
    capture_module_->Release();
    capture_module_ = NULL;
  }
  if (capture_thread_.Stop()) {
    
    delete &capture_thread_;
    delete &capture_event_;
    delete &deliver_event_;
  } else {
    assert(false);
    WEBRTC_TRACE(kTraceMemory, kTraceVideoRenderer,
                 ViEId(engine_id_, capture_id_),
                 "%s: Not able to stop capture thread for device %d, leaking",
                 __FUNCTION__, capture_id_);
  }

  if (image_proc_module_) {
    VideoProcessingModule::Destroy(image_proc_module_);
  }
  if (deflicker_frame_stats_) {
    delete deflicker_frame_stats_;
    deflicker_frame_stats_ = NULL;
  }
  delete brightness_frame_stats_;
  if (vcm_) {
    delete vcm_;
  }
}

ViECapturer* ViECapturer::CreateViECapture(
    int capture_id,
    int engine_id,
    VideoCaptureModule& capture_module,
    ProcessThread& module_process_thread) {
  ViECapturer* capture = new ViECapturer(capture_id, engine_id,
                                         module_process_thread);
  if (!capture || capture->Init(capture_module) != 0) {
    delete capture;
    capture = NULL;
  }
  return capture;
}

WebRtc_Word32 ViECapturer::Init(VideoCaptureModule& capture_module) {
  assert(capture_module_ == NULL);
  capture_module_ = &capture_module;
  capture_module_->RegisterCaptureDataCallback(*this);
  capture_module_->AddRef();
  if (module_process_thread_.RegisterModule(capture_module_) != 0) {
    return -1;
  }

  return 0;
}

ViECapturer* ViECapturer::CreateViECapture(
    int capture_id,
    int engine_id,
    const char* device_unique_idUTF8,
    const WebRtc_UWord32 device_unique_idUTF8Length,
    ProcessThread& module_process_thread) {
  ViECapturer* capture = new ViECapturer(capture_id, engine_id,
                                         module_process_thread);
  if (!capture ||
      capture->Init(device_unique_idUTF8, device_unique_idUTF8Length) != 0) {
    delete capture;
    capture = NULL;
  }
  return capture;
}

WebRtc_Word32 ViECapturer::Init(
    const char* device_unique_idUTF8,
    const WebRtc_UWord32 device_unique_idUTF8Length) {
  assert(capture_module_ == NULL);
  if (device_unique_idUTF8 == NULL) {
    capture_module_  = VideoCaptureFactory::Create(
        ViEModuleId(engine_id_, capture_id_), external_capture_module_);
  } else {
    capture_module_ = VideoCaptureFactory::Create(
        ViEModuleId(engine_id_, capture_id_), device_unique_idUTF8);
  }
  if (!capture_module_) {
    return -1;
  }
  capture_module_->AddRef();
  capture_module_->RegisterCaptureDataCallback(*this);
  if (module_process_thread_.RegisterModule(capture_module_) != 0) {
    return -1;
  }

  return 0;
}

int ViECapturer::FrameCallbackChanged() {
  if (Started() && !EncoderActive() && !CaptureCapabilityFixed()) {
    
    
    int best_width;
    int best_height;
    int best_frame_rate;
    VideoCaptureCapability capture_settings;
    capture_module_->CaptureSettings(capture_settings);
    GetBestFormat(best_width, best_height, best_frame_rate);
    if (best_width != 0 && best_height != 0 && best_frame_rate != 0) {
      if (best_width != capture_settings.width ||
          best_height != capture_settings.height ||
          best_frame_rate != capture_settings.maxFPS ||
          capture_settings.codecType != kVideoCodecUnknown) {
        Stop();
        Start(requested_capability_);
      }
    }
  }
  return 0;
}

WebRtc_Word32 ViECapturer::Start(const CaptureCapability& capture_capability) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_), "%s",
               __FUNCTION__);
  int width;
  int height;
  int frame_rate;
  VideoCaptureCapability capability;
  requested_capability_ = capture_capability;
  if (EncoderActive()) {
    CriticalSectionScoped cs(encoding_cs_.get());
    capability.width = codec_.width;
    capability.height = codec_.height;
    capability.maxFPS = codec_.maxFramerate;
    capability.codecType = codec_.codecType;
    capability.rawType = kVideoI420;

  } else if (!CaptureCapabilityFixed()) {
    
    GetBestFormat(width, height, frame_rate);
    if (width == 0) {
      width = kViECaptureDefaultWidth;
    }
    if (height == 0) {
      height = kViECaptureDefaultHeight;
    }
    if (frame_rate == 0) {
      frame_rate = kViECaptureDefaultFramerate;
    }
    capability.height = height;
    capability.width = width;
    capability.maxFPS = frame_rate;
    capability.rawType = kVideoI420;
    capability.codecType = kVideoCodecUnknown;
  } else {
    
    
    capability.width = requested_capability_.width;
    capability.height = requested_capability_.height;
    capability.maxFPS = requested_capability_.maxFPS;
    capability.rawType = requested_capability_.rawType;
    capability.interlaced = requested_capability_.interlaced;
  }
  return capture_module_->StartCapture(capability);
}

WebRtc_Word32 ViECapturer::Stop() {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_), "%s",
               __FUNCTION__);
  requested_capability_ = CaptureCapability();
  return capture_module_->StopCapture();
}

bool ViECapturer::Started() {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_), "%s",
               __FUNCTION__);
  return capture_module_->CaptureStarted();
}

const char* ViECapturer::CurrentDeviceName() const {
  return capture_module_->CurrentDeviceName();
}

WebRtc_Word32 ViECapturer::SetCaptureDelay(WebRtc_Word32 delay_ms) {
  return capture_module_->SetCaptureDelay(delay_ms);
}

WebRtc_Word32 ViECapturer::SetRotateCapturedFrames(
  const RotateCapturedFrame rotation) {
  VideoCaptureRotation converted_rotation = kCameraRotate0;
  switch (rotation) {
    case RotateCapturedFrame_0:
      converted_rotation = kCameraRotate0;
      break;
    case RotateCapturedFrame_90:
      converted_rotation = kCameraRotate90;
      break;
    case RotateCapturedFrame_180:
      converted_rotation = kCameraRotate180;
      break;
    case RotateCapturedFrame_270:
      converted_rotation = kCameraRotate270;
      break;
  }
  return capture_module_->SetCaptureRotation(converted_rotation);
}

int ViECapturer::IncomingFrame(unsigned char* video_frame,
                               unsigned int video_frame_length,
                               unsigned short width,
                               unsigned short height,
                               RawVideoType video_type,
                               unsigned long long capture_time) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "ExternalCapture::IncomingFrame width %d, height %d, "
               "capture_time %u", width, height, capture_time);

  if (!external_capture_module_) {
    return -1;
  }
  VideoCaptureCapability capability;
  capability.width = width;
  capability.height = height;
  capability.rawType = video_type;
  return external_capture_module_->IncomingFrame(video_frame,
                                                 video_frame_length,
                                                 capability, capture_time);
}

int ViECapturer::IncomingFrameI420(const ViEVideoFrameI420& video_frame,
                                   unsigned long long capture_time) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "ExternalCapture::IncomingFrame width %d, height %d, "
               " capture_time %u", video_frame.width, video_frame.height,
               capture_time);

  if (!external_capture_module_) {
    return -1;
  }

  VideoFrameI420 frame;
  frame.width = video_frame.width;
  frame.height = video_frame.height;
  frame.y_plane = video_frame.y_plane;
  frame.u_plane = video_frame.u_plane;
  frame.v_plane = video_frame.v_plane;
  frame.y_pitch = video_frame.y_pitch;
  frame.u_pitch = video_frame.u_pitch;
  frame.v_pitch = video_frame.v_pitch;

  return external_capture_module_->IncomingFrameI420(frame, capture_time);
}

void ViECapturer::OnIncomingCapturedFrame(const WebRtc_Word32 capture_id,
                                          VideoFrame& video_frame,
                                          VideoCodecType codec_type) {
  WEBRTC_TRACE(kTraceStream, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_id: %d)", __FUNCTION__, capture_id);

  CriticalSectionScoped cs(capture_cs_.get());
  if (codec_type != kVideoCodecUnknown) {
    if (encoded_frame_.Length() != 0) {
      
      deliver_event_.Reset();
      WEBRTC_TRACE(kTraceWarning, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s(capture_id: %d) Last encoded frame not yet delivered.",
                   __FUNCTION__, capture_id);
      capture_cs_->Leave();
      
      deliver_event_.Wait(kMaxDeliverWaitTime);
      assert(encoded_frame_.Length() == 0);
      capture_cs_->Enter();
    }
    encoded_frame_.SwapFrame(video_frame);
  } else {
    captured_frame_.SwapFrame(video_frame);
  }
  capture_event_.Set();
  return;
}

void ViECapturer::OnCaptureDelayChanged(const WebRtc_Word32 id,
                                        const WebRtc_Word32 delay) {
  WEBRTC_TRACE(kTraceStream, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_id: %d) delay %d", __FUNCTION__, capture_id_,
               delay);

  
  ViEFrameProviderBase::SetFrameDelay(delay);
  CriticalSectionScoped cs(encoding_cs_.get());
  if (vie_encoder_) {
    vie_encoder_->DelayChanged(id, delay);
  }
}

WebRtc_Word32 ViECapturer::RegisterEffectFilter(
    ViEEffectFilter* effect_filter) {
  CriticalSectionScoped cs(deliver_cs_.get());

  if (!effect_filter) {
    if (!effect_filter_) {
      WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: no effect filter added for capture device %d",
                   __FUNCTION__, capture_id_);
      return -1;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
                 "%s: deregister effect filter for device %d", __FUNCTION__,
                 capture_id_);
  } else {
    if (effect_filter_) {
      WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: effect filter already added for capture device %d",
                   __FUNCTION__, capture_id_);
      return -1;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
                 "%s: register effect filter for device %d", __FUNCTION__,
                 capture_id_);
  }
  effect_filter_ = effect_filter;
  return 0;
}

WebRtc_Word32 ViECapturer::IncImageProcRefCount() {
  if (!image_proc_module_) {
    assert(image_proc_module_ref_counter_ == 0);
    image_proc_module_ = VideoProcessingModule::Create(
        ViEModuleId(engine_id_, capture_id_));
    if (!image_proc_module_) {
      WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: could not create video processing module",
                   __FUNCTION__);
      return -1;
    }
  }
  image_proc_module_ref_counter_++;
  return 0;
}

WebRtc_Word32 ViECapturer::DecImageProcRefCount() {
  image_proc_module_ref_counter_--;
  if (image_proc_module_ref_counter_ == 0) {
    
    VideoProcessingModule::Destroy(image_proc_module_);
    image_proc_module_ = NULL;
  }
  return 0;
}

WebRtc_Word32 ViECapturer::EnableDenoising(bool enable) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d, enable: %d)", __FUNCTION__,
               capture_id_, enable);

  CriticalSectionScoped cs(deliver_cs_.get());
  if (enable) {
    if (denoising_enabled_) {
      
      return 0;
    }
    denoising_enabled_ = true;
    if (IncImageProcRefCount() != 0) {
      return -1;
    }
  } else {
    if (denoising_enabled_ == false) {
      
      return 0;
    }
    denoising_enabled_ = false;
    DecImageProcRefCount();
  }

  return 0;
}

WebRtc_Word32 ViECapturer::EnableDeflickering(bool enable) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d, enable: %d)", __FUNCTION__,
               capture_id_, enable);

  CriticalSectionScoped cs(deliver_cs_.get());
  if (enable) {
    if (deflicker_frame_stats_) {
      WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: deflickering already enabled", __FUNCTION__);
      return -1;
    }
    if (IncImageProcRefCount() != 0) {
      return -1;
    }
    deflicker_frame_stats_ = new VideoProcessingModule::FrameStats();
  } else {
    if (deflicker_frame_stats_ == NULL) {
      WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: deflickering not enabled", __FUNCTION__);
      return -1;
    }
    DecImageProcRefCount();
    delete deflicker_frame_stats_;
    deflicker_frame_stats_ = NULL;
  }
  return 0;
}

WebRtc_Word32 ViECapturer::EnableBrightnessAlarm(bool enable) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d, enable: %d)", __FUNCTION__,
               capture_id_, enable);

  CriticalSectionScoped cs(deliver_cs_.get());
  if (enable) {
    if (brightness_frame_stats_) {
      WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: BrightnessAlarm already enabled", __FUNCTION__);
      return -1;
    }
    if (IncImageProcRefCount() != 0) {
      return -1;
    }
    brightness_frame_stats_ = new VideoProcessingModule::FrameStats();
  } else {
    DecImageProcRefCount();
    if (brightness_frame_stats_ == NULL) {
      WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: deflickering not enabled", __FUNCTION__);
      return -1;
    }
    delete brightness_frame_stats_;
    brightness_frame_stats_ = NULL;
  }
  return 0;
}

bool ViECapturer::ViECaptureThreadFunction(void* obj) {
  return static_cast<ViECapturer*>(obj)->ViECaptureProcess();
}

bool ViECapturer::ViECaptureProcess() {
  if (capture_event_.Wait(kThreadWaitTimeMs) == kEventSignaled) {
    deliver_cs_->Enter();
    if (captured_frame_.Length() > 0) {
      
      capture_cs_->Enter();
      deliver_frame_.SwapFrame(captured_frame_);
      captured_frame_.SetLength(0);
      capture_cs_->Leave();
      DeliverI420Frame(deliver_frame_);
    }
    if (encoded_frame_.Length() > 0) {
      capture_cs_->Enter();
      deliver_frame_.SwapFrame(encoded_frame_);
      encoded_frame_.SetLength(0);
      deliver_event_.Set();
      capture_cs_->Leave();
      DeliverCodedFrame(deliver_frame_);
    }
    deliver_cs_->Leave();
    if (current_brightness_level_ != reported_brightness_level_) {
      CriticalSectionScoped cs(observer_cs_.get());
      if (observer_) {
        observer_->BrightnessAlarm(id_, current_brightness_level_);
        reported_brightness_level_ = current_brightness_level_;
      }
    }
  }
  
  return true;
}

void ViECapturer::DeliverI420Frame(VideoFrame& video_frame) {
  
  if (deflicker_frame_stats_) {
    if (image_proc_module_->GetFrameStats(*deflicker_frame_stats_,
                                          video_frame) == 0) {
      image_proc_module_->Deflickering(video_frame, *deflicker_frame_stats_);
    } else {
      WEBRTC_TRACE(kTraceStream, kTraceVideo, ViEId(engine_id_, capture_id_),
                   "%s: could not get frame stats for captured frame",
                   __FUNCTION__);
    }
  }
  if (denoising_enabled_) {
    image_proc_module_->Denoising(video_frame);
  }
  if (brightness_frame_stats_) {
    if (image_proc_module_->GetFrameStats(*brightness_frame_stats_,
                                          video_frame) == 0) {
      WebRtc_Word32 brightness = image_proc_module_->BrightnessDetection(
          video_frame, *brightness_frame_stats_);

      switch (brightness) {
      case VideoProcessingModule::kNoWarning:
        current_brightness_level_ = Normal;
        break;
      case VideoProcessingModule::kDarkWarning:
        current_brightness_level_ = Dark;
        break;
      case VideoProcessingModule::kBrightWarning:
        current_brightness_level_ = Bright;
        break;
      default:
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
                     "%s: Brightness detection failed", __FUNCTION__);
      }
    }
  }
  if (effect_filter_) {
    effect_filter_->Transform(video_frame.Length(), video_frame.Buffer(),
                              video_frame.TimeStamp(), video_frame.Width(),
                              video_frame.Height());
  }
  
  ViEFrameProviderBase::DeliverFrame(video_frame);
}

void ViECapturer::DeliverCodedFrame(VideoFrame& video_frame) {
  if (encode_complete_callback_) {
    EncodedImage encoded_image(video_frame.Buffer(), video_frame.Length(),
                               video_frame.Size());
    encoded_image._timeStamp = 90 * (WebRtc_UWord32) video_frame.RenderTimeMs();
    encode_complete_callback_->Encoded(encoded_image);
  }

  if (NumberOfRegisteredFrameCallbacks() > 0 && decoder_initialized_) {
    video_frame.Swap(decode_buffer_.payloadData, decode_buffer_.bufferSize,
                     decode_buffer_.payloadSize);
    decode_buffer_.encodedHeight = video_frame.Height();
    decode_buffer_.encodedWidth = video_frame.Width();
    decode_buffer_.renderTimeMs = video_frame.RenderTimeMs();
    decode_buffer_.timeStamp = 90 * (WebRtc_UWord32) video_frame.RenderTimeMs();
    decode_buffer_.payloadType = codec_.plType;
    vcm_->DecodeFromStorage(decode_buffer_);
  }
}

int ViECapturer::DeregisterFrameCallback(
    const ViEFrameCallback* callbackObject) {
  provider_cs_->Enter();
  if (callbackObject == vie_encoder_) {
    
    ViEEncoder* vie_encoder = NULL;
    vie_encoder = vie_encoder_;
    vie_encoder_ = NULL;
    provider_cs_->Leave();

    
    
    deliver_cs_->Enter();
    vie_encoder->DeRegisterExternalEncoder(codec_.plType);
    deliver_cs_->Leave();
    return 0;
  }
  provider_cs_->Leave();
  return ViEFrameProviderBase::DeregisterFrameCallback(callbackObject);
}

bool ViECapturer::IsFrameCallbackRegistered(
    const ViEFrameCallback* callbackObject) {
  CriticalSectionScoped cs(provider_cs_.get());
  if (callbackObject == vie_encoder_) {
    return true;
  }
  return ViEFrameProviderBase::IsFrameCallbackRegistered(callbackObject);
}

WebRtc_Word32 ViECapturer::PreEncodeToViEEncoder(const VideoCodec& codec,
                                                 ViEEncoder& vie_encoder,
                                                 WebRtc_Word32 vie_encoder_id) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d)", __FUNCTION__, capture_id_);
  if (vie_encoder_ && &vie_encoder != vie_encoder_) {
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
                 "%s(capture_device_id: %d Capture device already encoding)",
                 __FUNCTION__, capture_id_);
    return -1;
  }

  CriticalSectionScoped cs(encoding_cs_.get());
  VideoCaptureModule::VideoCaptureEncodeInterface* capture_encoder =
    capture_module_->GetEncodeInterface(codec);
  if (!capture_encoder) {
    
    return -1;
  }
  capture_encoder_ = capture_encoder;

  
  if (!vcm_) {
    vcm_ = VideoCodingModule::Create(capture_id_);
  }

  if (vie_encoder.RegisterExternalEncoder(this, codec.plType) != 0) {
    return -1;
  }
  if (vie_encoder.SetEncoder(codec) != 0) {
    vie_encoder.DeRegisterExternalEncoder(codec.plType);
    return -1;
  }

  
  ViEFrameProviderBase::DeregisterFrameCallback(&vie_encoder);
  
  vie_encoder_ = &vie_encoder;
  vie_encoder_id_ = vie_encoder_id;
  memcpy(&codec_, &codec, sizeof(VideoCodec));
  return 0;
}

bool ViECapturer::EncoderActive() {
  return vie_encoder_ != NULL;
}

bool ViECapturer::CaptureCapabilityFixed() {
  return requested_capability_.width != 0 &&
      requested_capability_.height != 0 &&
      requested_capability_.maxFPS != 0;
}

WebRtc_Word32 ViECapturer::Version(char* version,
                                   WebRtc_Word32 length) const {
  return 0;
}

WebRtc_Word32 ViECapturer::InitEncode(const VideoCodec* codec_settings,
                                      WebRtc_Word32 number_of_cores,
                                      WebRtc_UWord32 max_payload_size) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d)", __FUNCTION__, capture_id_);

  CriticalSectionScoped cs(encoding_cs_.get());
  if (!capture_encoder_ || !codec_settings) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (vcm_) {
    
    if (vcm_->InitializeReceiver() == 0) {
      if (vcm_->RegisterReceiveCallback(this) == 0) {
        if (vcm_->RegisterReceiveCodec(codec_settings, number_of_cores,
                                       false) == 0) {
          decoder_initialized_ = true;
          WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
                       "%s(capture_device_id: %d) VCM Decoder initialized",
                       __FUNCTION__, capture_id_);
        }
      }
    }
  }
  return capture_encoder_->ConfigureEncoder(*codec_settings, max_payload_size);
}

WebRtc_Word32 ViECapturer::Encode(const RawImage& input_image,
                                  const CodecSpecificInfo* codec_specific_info,
                                  const VideoFrameType frame_type) {
  CriticalSectionScoped cs(encoding_cs_.get());
  if (!capture_encoder_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (frame_type == kKeyFrame) {
    return capture_encoder_->EncodeFrameType(kVideoFrameKey);
  }
  if (frame_type == kSkipFrame) {
    return capture_encoder_->EncodeFrameType(kFrameEmpty);
  }
  return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
}

WebRtc_Word32 ViECapturer::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d)", __FUNCTION__, capture_id_);

  CriticalSectionScoped cs(deliver_cs_.get());
  if (!capture_encoder_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  encode_complete_callback_ = callback;
  return 0;
}

WebRtc_Word32 ViECapturer::Release() {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d)", __FUNCTION__, capture_id_);
  {
    CriticalSectionScoped cs(deliver_cs_.get());
    encode_complete_callback_ = NULL;
  }

  {
    CriticalSectionScoped cs(encoding_cs_.get());

    decoder_initialized_ = false;
    codec_.codecType = kVideoCodecUnknown;
    
    capture_encoder_->ConfigureEncoder(codec_, 0);

    if (vie_encoder_) {
      
      ViEFrameProviderBase::RegisterFrameCallback(vie_encoder_id_,
                                                  vie_encoder_);
    }
    vie_encoder_ = NULL;
  }
  return 0;
}



WebRtc_Word32 ViECapturer::Reset() {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d)", __FUNCTION__, capture_id_);
  return 0;
}

WebRtc_Word32 ViECapturer::SetChannelParameters(WebRtc_UWord32 packet_loss,
                                                int rtt) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d)", __FUNCTION__, capture_id_);

  CriticalSectionScoped cs(encoding_cs_.get());
  if (!capture_encoder_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  return capture_encoder_->SetChannelParameters(packet_loss, rtt);
}

WebRtc_Word32 ViECapturer::SetRates(WebRtc_UWord32 new_bit_rate,
                                    WebRtc_UWord32 frame_rate) {
  WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, capture_id_),
               "%s(capture_device_id: %d)", __FUNCTION__, capture_id_);

  CriticalSectionScoped cs(encoding_cs_.get());
  if (!capture_encoder_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  return capture_encoder_->SetRates(new_bit_rate, frame_rate);
}

WebRtc_Word32 ViECapturer::FrameToRender(VideoFrame& video_frame) {
  deliver_cs_->Enter();
  DeliverI420Frame(video_frame);
  deliver_cs_->Leave();
  return 0;
}

WebRtc_Word32 ViECapturer::RegisterObserver(ViECaptureObserver& observer) {
  if (observer_) {
    WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                 "%s Observer already registered", __FUNCTION__, capture_id_);
    return -1;
  }
  if (capture_module_->RegisterCaptureCallback(*this) != 0) {
    return -1;
  }
  capture_module_->EnableFrameRateCallback(true);
  capture_module_->EnableNoPictureAlarm(true);
  observer_ = &observer;
  return 0;
}

WebRtc_Word32 ViECapturer::DeRegisterObserver() {
  CriticalSectionScoped cs(observer_cs_.get());
  if (!observer_) {
    WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, capture_id_),
                 "%s No observer registered", __FUNCTION__, capture_id_);
    return -1;
  }
  capture_module_->EnableFrameRateCallback(false);
  capture_module_->EnableNoPictureAlarm(false);
  capture_module_->DeRegisterCaptureCallback();
  observer_ = NULL;
  return 0;
}

bool ViECapturer::IsObserverRegistered() {
  CriticalSectionScoped cs(observer_cs_.get());
  return observer_ != NULL;
}

void ViECapturer::OnCaptureFrameRate(const WebRtc_Word32 id,
                                     const WebRtc_UWord32 frame_rate) {
  WEBRTC_TRACE(kTraceStream, kTraceVideo, ViEId(engine_id_, capture_id_),
               "OnCaptureFrameRate %d", frame_rate);

  CriticalSectionScoped cs(observer_cs_.get());
  observer_->CapturedFrameRate(id_, (WebRtc_UWord8) frame_rate);
}

void ViECapturer::OnNoPictureAlarm(const WebRtc_Word32 id,
                                   const VideoCaptureAlarm alarm) {
  WEBRTC_TRACE(kTraceStream, kTraceVideo, ViEId(engine_id_, capture_id_),
               "OnNoPictureAlarm %d", alarm);

  CriticalSectionScoped cs(observer_cs_.get());
  CaptureAlarm vie_alarm = (alarm == Raised) ? AlarmRaised : AlarmCleared;
  observer_->NoPictureAlarm(id, vie_alarm);
}

WebRtc_Word32 ViECapturer::SetCaptureDeviceImage(
    const VideoFrame& capture_device_image) {
  return capture_module_->StartSendImage(capture_device_image, 10);
}

}  
