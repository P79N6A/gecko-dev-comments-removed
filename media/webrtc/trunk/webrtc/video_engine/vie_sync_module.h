












#ifndef WEBRTC_VIDEO_ENGINE_VIE_SYNC_MODULE_H_
#define WEBRTC_VIDEO_ENGINE_VIE_SYNC_MODULE_H_

#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/video_engine/stream_synchronization.h"
#include "webrtc/voice_engine/include/voe_video_sync.h"

namespace webrtc {

class CriticalSectionWrapper;
class RtpRtcp;
class VideoCodingModule;
class ViEChannel;
class VoEVideoSync;

class ViESyncModule : public Module {
 public:
  ViESyncModule(VideoCodingModule* vcm,
                ViEChannel* vie_channel);
  ~ViESyncModule();

  int ConfigureSync(int voe_channel_id,
                    VoEVideoSync* voe_sync_interface,
                    RtpRtcp* video_rtcp_module);

  int VoiceChannel();

  
  int SetTargetBufferingDelay(int target_delay_ms);

  
  virtual int32_t TimeUntilNextProcess();
  virtual int32_t Process();

 private:
  scoped_ptr<CriticalSectionWrapper> data_cs_;
  VideoCodingModule* vcm_;
  ViEChannel* vie_channel_;
  RtpRtcp* video_rtp_rtcp_;
  int voe_channel_id_;
  VoEVideoSync* voe_sync_interface_;
  TickTime last_sync_time_;
  scoped_ptr<StreamSynchronization> sync_;
  StreamSynchronization::Measurements audio_measurement_;
  StreamSynchronization::Measurements video_measurement_;
};

}  

#endif  
