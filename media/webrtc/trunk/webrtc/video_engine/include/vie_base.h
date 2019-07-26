

















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_BASE_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_BASE_H_

#include "common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VideoEngine {
 public:
  
  static VideoEngine* Create();

  
  static bool Delete(VideoEngine*& video_engine);

  
  
  static int SetTraceFilter(const unsigned int filter);

  
  static int SetTraceFile(const char* file_nameUTF8,
                          const bool add_file_counter = false);

  
  
  static int SetTraceCallback(TraceCallback* callback);

  
  
  
  static int SetAndroidObjects(void* java_vm, void* java_context);

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
