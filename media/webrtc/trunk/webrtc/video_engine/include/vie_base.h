

















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_BASE_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_BASE_H_

#include "webrtc/common_types.h"

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
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


class WEBRTC_DLLEXPORT VideoEngine {
 public:
  
  static VideoEngine* Create();
  static VideoEngine* Create(const Config& config);

  
  static bool Delete(VideoEngine*& video_engine);

  
  
  static int SetTraceFilter(const unsigned int filter);

  
  static int SetTraceFile(const char* file_nameUTF8,
                          const bool add_file_counter = false);

  
  
  static int SetTraceCallback(TraceCallback* callback);

#if defined(ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)
  
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

  
  
  
  
  
  
  
  
  
  
  
  virtual int CpuOveruseMeasures(int channel,
                                 int* capture_jitter_ms,
                                 int* avg_encode_time_ms,
                                 int* encode_usage_percent,
                                 int* capture_queue_delay_ms_per_s) {
    return -1;
  }

  
  
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
