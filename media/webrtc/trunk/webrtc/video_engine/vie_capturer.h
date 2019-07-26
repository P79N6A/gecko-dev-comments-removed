









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
      uint32_t device_unique_idUTF8Length,
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

  
  
  virtual int32_t PreEncodeToViEEncoder(const VideoCodec& codec,
                                        ViEEncoder& vie_encoder,
                                        int32_t vie_encoder_id);

  
  int32_t Start(
      const CaptureCapability& capture_capability = CaptureCapability());
  int32_t Stop();
  bool Started();

  
  int32_t SetCaptureDelay(int32_t delay_ms);

  
  int32_t SetRotateCapturedFrames(const RotateCapturedFrame rotation);

  
  int32_t RegisterEffectFilter(ViEEffectFilter* effect_filter);
  int32_t EnableDenoising(bool enable);
  int32_t EnableDeflickering(bool enable);
  int32_t EnableBrightnessAlarm(bool enable);

  
  int32_t RegisterObserver(ViECaptureObserver* observer);
  int32_t DeRegisterObserver();
  bool IsObserverRegistered();

  
  const char* CurrentDeviceName() const;

 protected:
  ViECapturer(int capture_id,
              int engine_id,
              ProcessThread& module_process_thread);

  int32_t Init(VideoCaptureModule* capture_module);
  int32_t Init(const char* device_unique_idUTF8,
               const uint32_t device_unique_idUTF8Length);

  
  virtual void OnIncomingCapturedFrame(const int32_t id,
                                       I420VideoFrame& video_frame);
  virtual void OnIncomingCapturedEncodedFrame(const int32_t capture_id,
                                              VideoFrame& video_frame,
                                              VideoCodecType codec_type);
  virtual void OnCaptureDelayChanged(const int32_t id,
                                     const int32_t delay);

  bool EncoderActive();

  
  
  bool CaptureCapabilityFixed();

  
  
  
  int32_t IncImageProcRefCount();
  int32_t DecImageProcRefCount();

  
  virtual int32_t Version(char* version, int32_t length) const;
  virtual int32_t InitEncode(const VideoCodec* codec_settings,
                             int32_t number_of_cores,
                             uint32_t max_payload_size);
  virtual int32_t Encode(const I420VideoFrame& input_image,
                         const CodecSpecificInfo* codec_specific_info,
                         const std::vector<VideoFrameType>* frame_types);
  virtual int32_t RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback);
  virtual int32_t Release();
  virtual int32_t Reset();
  virtual int32_t SetChannelParameters(uint32_t packet_loss, int rtt);
  virtual int32_t SetRates(uint32_t new_bit_rate, uint32_t frame_rate);

  
  
  virtual int32_t FrameToRender(I420VideoFrame& video_frame);  

  
  virtual void OnCaptureFrameRate(const int32_t id,
                                  const uint32_t frame_rate);
  virtual void OnNoPictureAlarm(const int32_t id,
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
  
  int32_t vie_encoder_id_;
  
  VideoCodingModule* vcm_;
  EncodedVideoData decode_buffer_;
  bool decoder_initialized_;
  CaptureCapability requested_capability_;

  I420VideoFrame capture_device_image_;
};

}  

#endif  
