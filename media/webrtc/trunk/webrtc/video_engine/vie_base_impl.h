









#ifndef WEBRTC_VIDEO_ENGINE_VIE_BASE_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_BASE_IMPL_H_

#include "webrtc/video_engine/include/vie_base.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/video_engine/vie_ref_count.h"
#include "webrtc/video_engine/vie_shared_data.h"

namespace webrtc {

class Config;
class Module;
class VoiceEngine;

class ViEBaseImpl
    : public ViEBase,
      public ViERefCount {
 public:
  virtual int Release();

  
  virtual int Init();
  virtual int SetVoiceEngine(VoiceEngine* voice_engine);
  virtual int RegisterCpuOveruseObserver(int channel,
                                         CpuOveruseObserver* observer);
  virtual int CpuOveruseMeasures(int channel,
                                 int* capture_jitter_ms,
                                 int* avg_encode_time_ms,
                                 int* encode_usage_percent,
                                 int* capture_queue_delay_ms_per_s);
  virtual int CreateChannel(int& video_channel);  
  virtual int CreateChannel(int& video_channel,  
                            const Config* config);
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
  explicit ViEBaseImpl(const Config& config);
  virtual ~ViEBaseImpl();

  ViESharedData* shared_data() { return &shared_data_; }

 private:
  
  int32_t AddViEVersion(char* str) const;
  int32_t AddBuildInfo(char* str) const;
  int32_t AddExternalTransportBuild(char* str) const;

  int CreateChannel(int& video_channel, int original_channel,  
                    bool sender);

  
  ViESharedData shared_data_;
};

}  

#endif  
