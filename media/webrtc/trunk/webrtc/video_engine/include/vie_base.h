

















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_BASE_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_BASE_H_

#include "webrtc/common_types.h"

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD) && !defined(MOZ_WIDGET_GONK)
#include <jni.h>
#endif

namespace webrtc {

class Config;
class VoiceEngine;



class CpuOveruseObserver {
 public:
  
  virtual void OveruseDetected() = 0;
  
  virtual void NormalUsage() = 0;

 protected:
  virtual ~CpuOveruseObserver() {}
};

struct CpuOveruseOptions {
  CpuOveruseOptions()
      : enable_capture_jitter_method(true),
        low_capture_jitter_threshold_ms(20.0f),
        high_capture_jitter_threshold_ms(30.0f),
        enable_encode_usage_method(false),
        low_encode_usage_threshold_percent(60),
        high_encode_usage_threshold_percent(90),
        low_encode_time_rsd_threshold(-1),
        high_encode_time_rsd_threshold(-1),
        enable_extended_processing_usage(true),
        frame_timeout_interval_ms(1500),
        min_frame_samples(120),
        min_process_count(3),
        high_threshold_consecutive_count(2) {}

  
  bool enable_capture_jitter_method;
  float low_capture_jitter_threshold_ms;  
  float high_capture_jitter_threshold_ms; 
  
  bool enable_encode_usage_method;
  int low_encode_usage_threshold_percent;  
  int high_encode_usage_threshold_percent; 
  
  int low_encode_time_rsd_threshold;   
                                       
                                       
  int high_encode_time_rsd_threshold;  
                                       
                                       
  bool enable_extended_processing_usage;  
                                          
                                          
                                          
  
  int frame_timeout_interval_ms;  
                                  
  int min_frame_samples;  
  int min_process_count;  
                          
  int high_threshold_consecutive_count; 
                                        
                                        

  bool Equals(const CpuOveruseOptions& o) const {
    return enable_capture_jitter_method == o.enable_capture_jitter_method &&
        low_capture_jitter_threshold_ms == o.low_capture_jitter_threshold_ms &&
        high_capture_jitter_threshold_ms ==
        o.high_capture_jitter_threshold_ms &&
        enable_encode_usage_method == o.enable_encode_usage_method &&
        low_encode_usage_threshold_percent ==
        o.low_encode_usage_threshold_percent &&
        high_encode_usage_threshold_percent ==
        o.high_encode_usage_threshold_percent &&
        low_encode_time_rsd_threshold == o.low_encode_time_rsd_threshold &&
        high_encode_time_rsd_threshold == o.high_encode_time_rsd_threshold &&
        enable_extended_processing_usage ==
        o.enable_extended_processing_usage &&
        frame_timeout_interval_ms == o.frame_timeout_interval_ms &&
        min_frame_samples == o.min_frame_samples &&
        min_process_count == o.min_process_count &&
        high_threshold_consecutive_count == o.high_threshold_consecutive_count;
  }
};

struct CpuOveruseMetrics {
  CpuOveruseMetrics()
      : capture_jitter_ms(-1),
        avg_encode_time_ms(-1),
        encode_usage_percent(-1),
        encode_rsd(-1),
        capture_queue_delay_ms_per_s(-1) {}

  int capture_jitter_ms;  
                          
  int avg_encode_time_ms;   
  int encode_usage_percent; 
                            
  
  int encode_rsd;           
  int capture_queue_delay_ms_per_s;  
                                     
                                     
                                     
};

class WEBRTC_DLLEXPORT VideoEngine {
 public:
  
  static VideoEngine* Create();
  static VideoEngine* Create(const Config& config);

  
  static bool Delete(VideoEngine*& video_engine);

  
  
  static int SetTraceFilter(const unsigned int filter);

  
  static int SetTraceFile(const char* file_nameUTF8,
                          const bool add_file_counter = false);

  
  
  static int SetTraceCallback(TraceCallback* callback);

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD) && !defined(MOZ_WIDGET_GONK)
  
  static int SetAndroidObjects(JavaVM* java_vm);
#endif

 protected:
  VideoEngine() {}
  virtual ~VideoEngine() {}
};

class WEBRTC_DLLEXPORT ViEBase {
 public:
  
  
  
  static ViEBase* GetInterface(VideoEngine* video_engine);

  
  
  
  virtual int Release() = 0;

  
  virtual int Init() = 0;

  
  
  virtual int SetVoiceEngine(VoiceEngine* voice_engine) = 0;

  
  virtual int CreateChannel(int& video_channel) = 0;

  
  
  
  
  
  virtual int CreateChannel(int& video_channel,
                            int original_channel) = 0;

  
  
  
  virtual int CreateReceiveChannel(int& video_channel,
                                   int original_channel) = 0;

  
  virtual int DeleteChannel(const int video_channel) = 0;

  
  
  
  virtual int RegisterCpuOveruseObserver(int channel,
                                         CpuOveruseObserver* observer) = 0;

  
  virtual int SetCpuOveruseOptions(int channel,
                                   const CpuOveruseOptions& options) = 0;

  
  virtual int GetCpuOveruseMetrics(int channel, CpuOveruseMetrics* metrics) = 0;

  
  
  
  
  virtual void RegisterSendSideDelayObserver(
      int channel, SendSideDelayObserver* observer) {}

  
  
  virtual void SetLoadManager(CPULoadStateCallbackInvoker* load_manager) = 0;

  
  
  virtual int ConnectAudioChannel(const int video_channel,
                                  const int audio_channel) = 0;

  
  virtual int DisconnectAudioChannel(const int video_channel) = 0;

  
  
  virtual int StartSend(const int video_channel) = 0;

  
  virtual int StopSend(const int video_channel) = 0;

  
  virtual int StartReceive(const int video_channel) = 0;

  
  virtual int StopReceive(const int video_channel) = 0;

  
  virtual int GetVersion(char version[1024]) = 0;

  
  virtual int LastError() = 0;

 protected:
  ViEBase() {}
  virtual ~ViEBase() {}
};

}  

#endif  
