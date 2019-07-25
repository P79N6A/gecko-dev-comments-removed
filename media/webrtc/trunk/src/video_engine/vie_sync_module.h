












#ifndef WEBRTC_VIDEO_ENGINE_VIE_SYNC_MODULE_H_
#define WEBRTC_VIDEO_ENGINE_VIE_SYNC_MODULE_H_

#include "module.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "tick_util.h"

namespace webrtc {

class CriticalSectionWrapper;
class RtpRtcp;
class VideoCodingModule;
class VoEVideoSync;

class ViESyncModule : public Module {
 public:
  ViESyncModule(int id, VideoCodingModule& vcm, RtpRtcp& rtcp_module);
  ~ViESyncModule();

  int SetVoiceChannel(int voe_channel_id, VoEVideoSync* voe_sync_interface);
  int VoiceChannel();

  
  
  void SetNetworkDelay(int network_delay);

  
  virtual WebRtc_Word32 Version(char* version,
                                WebRtc_UWord32& remaining_buffer_in_bytes,
                                WebRtc_UWord32& position) const;
  virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);
  virtual WebRtc_Word32 TimeUntilNextProcess();
  virtual WebRtc_Word32 Process();

 private:
  scoped_ptr<CriticalSectionWrapper> data_cs_;
  int id_;
  VideoCodingModule& vcm_;
  RtpRtcp& rtcp_module_;
  int voe_channel_id_;
  VoEVideoSync* voe_sync_interface_;
  TickTime last_sync_time_;

  struct ViESyncDelay {
    ViESyncDelay() {
      extra_video_delay_ms = 0;
      last_video_delay_ms = 0;
      extra_audio_delay_ms = 0;
      last_sync_delay = 0;
      network_delay = 120;
    }
    int extra_video_delay_ms;
    int last_video_delay_ms;
    int extra_audio_delay_ms;
    int last_sync_delay;
    int network_delay;
  };
  ViESyncDelay channel_delay_;
};

}  

#endif  
