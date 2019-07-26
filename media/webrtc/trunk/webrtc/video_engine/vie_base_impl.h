









#ifndef WEBRTC_VIDEO_ENGINE_VIE_BASE_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_BASE_IMPL_H_

#include "video_engine/include/vie_base.h"
#include "video_engine/vie_defines.h"
#include "video_engine/vie_ref_count.h"
#include "video_engine/vie_shared_data.h"

namespace webrtc {

class Module;
class VoiceEngine;

class ViEBaseImpl
    : public ViEBase,
      public ViERefCount {
 public:
  virtual int Release();

  
  virtual int Init();
  virtual int SetVoiceEngine(VoiceEngine* voice_engine);
  virtual int CreateChannel(int& video_channel);  
  virtual int CreateChannel(int& video_channel,  
                            int original_channel);
  virtual int CreateReceiveChannel(int& video_channel,  
                                   int original_channel);
  virtual int DeleteChannel(const int video_channel);
  virtual int ConnectAudioChannel(const int video_channel,
                                  const int audio_channel);
  virtual int DisconnectAudioChannel(const int video_channel);
  virtual int StartSend(const int video_channel);
  virtual int StopSend(const int video_channel);
  virtual int StartReceive(const int video_channel);
  virtual int StopReceive(const int video_channel);
  virtual int GetVersion(char version[1024]);
  virtual int LastError();

 protected:
  ViEBaseImpl();
  virtual ~ViEBaseImpl();

  ViESharedData* shared_data() { return &shared_data_; }

 private:
  
  WebRtc_Word32 AddViEVersion(char* str) const;
  WebRtc_Word32 AddBuildInfo(char* str) const;
  WebRtc_Word32 AddExternalTransportBuild(char* str) const;

  int CreateChannel(int& video_channel, int original_channel,  
                    bool sender);

  
  ViESharedData shared_data_;
};

}  

#endif  
