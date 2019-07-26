









#ifndef WEBRTC_VIDEO_ENGINE_VIE_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CAPTURER_H_

#include <vector>

#include "common_types.h"  
#include "engine_configurations.h"  
#include "webrtc/modules/video_capture/include/video_capture.h"
#include "modules/video_coding/codecs/interface/video_codec_interface.h"
#include "modules/video_coding/main/interface/video_coding.h"
#include "modules/video_processing/main/interface/video_processing.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h" 
#include "video_engine/include/vie_capture.h"
#include "video_engine/vie_defines.h"
#include "video_engine/vie_frame_provider_base.h"

namespace webrtc {

class CriticalSectionWrapper;
class EventWrapper;
class ProcessThread;
class ThreadWrapper;
class ViEEffectFilter;
class ViEEncoder;
struct ViEPicture;

class ViECapturer
    : public ViEFrameProviderBase,
      public ViEExternalCapture,
      protected VCMReceiveCallback,
      protected VideoCaptureDataCallback,
      protected VideoCaptureFeedBack,
      protected VideoEncoder {
 public:
  static ViECapturer* CreateViECapture(int capture_id,
                                       int engine_id,
                                       VideoCaptureModule* capture_module,
                                       ProcessThread& module_process_thread);

  static ViECapturer* CreateViECapture(
      int capture_id,
      int engine_id,
      const char* device_unique_idUTF8,
      WebRtc_UWord32 device_unique_idUTF8Length,
      ProcessThread& module_process_thread);

  ~ViECapturer();

  
  int FrameCallbackChanged();
  virtual int DeregisterFrameCallback(const ViEFrameCallback* callbackObject);
  bool IsFrameCallbackRegistered(const ViEFrameCallback* callbackObject);

  
  virtual int IncomingFrame(unsigned char* video_frame,
                            unsigned int video_frame_length,
                            uint16_t width,
                            uint16_t height,
                            RawVideoType video_type,
                            unsigned long long capture_time = 0);  

  virtual int IncomingFrameI420(const ViEVideoFrameI420& video_frame,
                                unsigned long long capture_time = 0);  

  
  
  virtual WebRtc_Word32 PreEncodeToViEEncoder(const VideoCodec& codec,
                                              ViEEncoder& vie_encoder,
                                              WebRtc_Word32 vie_encoder_id);

  
  WebRtc_Word32 Start(
      const CaptureCapability& capture_capability = CaptureCapability());
  WebRtc_Word32 Stop();
  bool Started();

  
  WebRtc_Word32 SetCaptureDelay(WebRtc_Word32 delay_ms);

  
  WebRtc_Word32 SetRotateCapturedFrames(const RotateCapturedFrame rotation);

  
  WebRtc_Word32 RegisterEffectFilter(ViEEffectFilter* effect_filter);
  WebRtc_Word32 EnableDenoising(bool enable);
  WebRtc_Word32 EnableDeflickering(bool enable);
  WebRtc_Word32 EnableBrightnessAlarm(bool enable);

  
  WebRtc_Word32 RegisterObserver(ViECaptureObserver* observer);
  WebRtc_Word32 DeRegisterObserver();
  bool IsObserverRegistered();

  
  const char* CurrentDeviceName() const;

 protected:
  ViECapturer(int capture_id,
              int engine_id,
              ProcessThread& module_process_thread);

  WebRtc_Word32 Init(VideoCaptureModule* capture_module);
  WebRtc_Word32 Init(const char* device_unique_idUTF8,
                     const WebRtc_UWord32 device_unique_idUTF8Length);

  
  virtual void OnIncomingCapturedFrame(const WebRtc_Word32 id,
                                       I420VideoFrame& video_frame);
  virtual void OnIncomingCapturedEncodedFrame(const WebRtc_Word32 capture_id,
                                              VideoFrame& video_frame,
                                              VideoCodecType codec_type);
  virtual void OnCaptureDelayChanged(const WebRtc_Word32 id,
                                     const WebRtc_Word32 delay);

  bool EncoderActive();

  
  
  bool CaptureCapabilityFixed();

  
  
  
  WebRtc_Word32 IncImageProcRefCount();
  WebRtc_Word32 DecImageProcRefCount();

  
  virtual WebRtc_Word32 Version(char* version,
                                WebRtc_Word32 length) const;
  virtual WebRtc_Word32 InitEncode(const VideoCodec* codec_settings,
                                   WebRtc_Word32 number_of_cores,
                                   WebRtc_UWord32 max_payload_size);
  virtual WebRtc_Word32 Encode(const I420VideoFrame& input_image,
                               const CodecSpecificInfo* codec_specific_info,
                               const std::vector<VideoFrameType>* frame_types);
  virtual WebRtc_Word32 RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback);
  virtual WebRtc_Word32 Release();
  virtual WebRtc_Word32 Reset();
  virtual WebRtc_Word32 SetChannelParameters(WebRtc_UWord32 packet_loss,
                                             int rtt);
  virtual WebRtc_Word32 SetRates(WebRtc_UWord32 new_bit_rate,
                                 WebRtc_UWord32 frame_rate);

  
  
  virtual WebRtc_Word32 FrameToRender(I420VideoFrame& video_frame);  

  
  virtual void OnCaptureFrameRate(const WebRtc_Word32 id,
                                  const WebRtc_UWord32 frame_rate);
  virtual void OnNoPictureAlarm(const WebRtc_Word32 id,
                                const VideoCaptureAlarm alarm);

  
  static bool ViECaptureThreadFunction(void* obj);
  bool ViECaptureProcess();

  void DeliverI420Frame(I420VideoFrame* video_frame);
  void DeliverCodedFrame(VideoFrame* video_frame);

 private:
  
  scoped_ptr<CriticalSectionWrapper> capture_cs_;
  scoped_ptr<CriticalSectionWrapper> deliver_cs_;
  VideoCaptureModule* capture_module_;
  VideoCaptureExternal* external_capture_module_;
  ProcessThread& module_process_thread_;
  const int capture_id_;

  
  ThreadWrapper& capture_thread_;
  EventWrapper& capture_event_;
  EventWrapper& deliver_event_;

  I420VideoFrame captured_frame_;
  I420VideoFrame deliver_frame_;
  VideoFrame deliver_encoded_frame_;
  VideoFrame encoded_frame_;

  
  ViEEffectFilter* effect_filter_;
  VideoProcessingModule* image_proc_module_;
  int image_proc_module_ref_counter_;
  VideoProcessingModule::FrameStats* deflicker_frame_stats_;
  VideoProcessingModule::FrameStats* brightness_frame_stats_;
  Brightness current_brightness_level_;
  Brightness reported_brightness_level_;
  bool denoising_enabled_;

  
  scoped_ptr<CriticalSectionWrapper> observer_cs_;
  ViECaptureObserver* observer_;

  
  scoped_ptr<CriticalSectionWrapper> encoding_cs_;
  VideoCaptureModule::VideoCaptureEncodeInterface* capture_encoder_;
  EncodedImageCallback* encode_complete_callback_;
  VideoCodec codec_;
  
  ViEEncoder* vie_encoder_;
  
  WebRtc_Word32 vie_encoder_id_;
  
  VideoCodingModule* vcm_;
  EncodedVideoData decode_buffer_;
  bool decoder_initialized_;
  CaptureCapability requested_capability_;

  I420VideoFrame capture_device_image_;
};

}  

#endif  
