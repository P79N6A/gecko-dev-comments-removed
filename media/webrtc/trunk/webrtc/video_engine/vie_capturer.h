









#ifndef WEBRTC_VIDEO_ENGINE_VIE_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CAPTURER_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/video_capture/include/video_capture.h"
#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/thread_annotations.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_engine/include/vie_capture.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/video_engine/vie_frame_provider_base.h"

namespace webrtc {

class Config;
class CriticalSectionWrapper;
class EventWrapper;
class CpuOveruseObserver;
class OveruseFrameDetector;
class ProcessThread;
class ThreadWrapper;
class ViEEffectFilter;
class ViEEncoder;
struct ViEPicture;

class ViECapturer
    : public ViEFrameProviderBase,
      public ViEExternalCapture,
      protected VideoCaptureDataCallback,
      protected VideoCaptureFeedBack {
 public:
  static ViECapturer* CreateViECapture(int capture_id,
                                       int engine_id,
                                       const Config& config,
                                       VideoCaptureModule* capture_module,
                                       ProcessThread& module_process_thread);

  static ViECapturer* CreateViECapture(
      int capture_id,
      int engine_id,
      const Config& config,
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

  virtual void SwapFrame(I420VideoFrame* frame) OVERRIDE;

  
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

  void RegisterCpuOveruseObserver(CpuOveruseObserver* observer);

  void CpuOveruseMeasures(int* capture_jitter_ms,
                          int* avg_encode_time_ms,
                          int* encode_usage_percent,
                          int* capture_queue_delay_ms_per_s) const;

 protected:
  ViECapturer(int capture_id,
              int engine_id,
              const Config& config,
              ProcessThread& module_process_thread);

  int32_t Init(VideoCaptureModule* capture_module);
  int32_t Init(const char* device_unique_idUTF8,
               uint32_t device_unique_idUTF8Length);

  
  virtual void OnIncomingCapturedFrame(const int32_t id,
                                       I420VideoFrame& video_frame);
  virtual void OnCaptureDelayChanged(const int32_t id,
                                     const int32_t delay);

  
  
  bool CaptureCapabilityFixed();

  
  
  
  int32_t IncImageProcRefCount();
  int32_t DecImageProcRefCount();

  
  virtual void OnCaptureFrameRate(const int32_t id,
                                  const uint32_t frame_rate);
  virtual void OnNoPictureAlarm(const int32_t id,
                                const VideoCaptureAlarm alarm);

  
  static bool ViECaptureThreadFunction(void* obj);
  bool ViECaptureProcess();

  void DeliverI420Frame(I420VideoFrame* video_frame);
  void DeliverCodedFrame(VideoFrame* video_frame);

 private:
  bool SwapCapturedAndDeliverFrameIfAvailable();

  
  scoped_ptr<CriticalSectionWrapper> capture_cs_;
  scoped_ptr<CriticalSectionWrapper> deliver_cs_;
  VideoCaptureModule* capture_module_;
  VideoCaptureExternal* external_capture_module_;
  ProcessThread& module_process_thread_;
  const int capture_id_;

  
  scoped_ptr<CriticalSectionWrapper> incoming_frame_cs_;
  I420VideoFrame incoming_frame_;

  
  ThreadWrapper& capture_thread_;
  EventWrapper& capture_event_;
  EventWrapper& deliver_event_;

  I420VideoFrame captured_frame_;
  I420VideoFrame deliver_frame_;

  
  ViEEffectFilter* effect_filter_;
  VideoProcessingModule* image_proc_module_;
  int image_proc_module_ref_counter_;
  VideoProcessingModule::FrameStats* deflicker_frame_stats_;
  VideoProcessingModule::FrameStats* brightness_frame_stats_;
  Brightness current_brightness_level_;
  Brightness reported_brightness_level_;
  bool denoising_enabled_;

  
  scoped_ptr<CriticalSectionWrapper> observer_cs_;
  ViECaptureObserver* observer_ GUARDED_BY(observer_cs_.get());

  CaptureCapability requested_capability_;

  scoped_ptr<OveruseFrameDetector> overuse_detector_;
};

}  

#endif  
