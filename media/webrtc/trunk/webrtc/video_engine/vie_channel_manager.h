









#ifndef WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_MANAGER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_MANAGER_H_

#include <list>
#include <map>

#include "webrtc/engine_configurations.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"
#include "webrtc/video_engine/vie_channel_group.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/video_engine/vie_manager_base.h"
#include "webrtc/video_engine/vie_remb.h"

namespace webrtc {

class Config;
class CriticalSectionWrapper;
class ProcessThread;
class RtcpRttStats;
class ViEChannel;
class ViEEncoder;
class VoEVideoSync;
class VoiceEngine;

typedef std::list<ChannelGroup*> ChannelGroups;
typedef std::list<ViEChannel*> ChannelList;
typedef std::map<int, ViEChannel*> ChannelMap;
typedef std::map<int, ViEEncoder*> EncoderMap;

class ViEChannelManager: private ViEManagerBase {
  friend class ViEChannelManagerScoped;
 public:
  ViEChannelManager(int engine_id,
                    int number_of_cores,
                    const Config& config);
  ~ViEChannelManager();

  void SetModuleProcessThread(ProcessThread* module_process_thread);

  void SetLoadManager(CPULoadStateCallbackInvoker* load_manager);

  
  int CreateChannel(int* channel_id,
                    const Config* config);

  
  
  
  int CreateChannel(int* channel_id, int original_channel, bool sender);

  
  int DeleteChannel(int channel_id);

  
  int SetVoiceEngine(VoiceEngine* voice_engine);

  
  int ConnectVoiceChannel(int channel_id, int audio_channel_id);

  
  int DisconnectVoiceChannel(int channel_id);

  VoiceEngine* GetVoiceEngine();

  
  bool SetRembStatus(int channel_id, bool sender, bool receiver);

  
  
  void UpdateSsrcs(int channel_id, const std::list<unsigned int>& ssrcs);

 private:
  
  
  bool CreateChannelObject(int channel_id,
                           ViEEncoder* vie_encoder,
                           RtcpBandwidthObserver* bandwidth_observer,
                           RemoteBitrateEstimator* remote_bitrate_estimator,
                           RtcpRttStats* rtcp_rtt_stats,
                           RtcpIntraFrameObserver* intra_frame_observer,
                           bool sender);

  
  
  ViEChannel* ViEChannelPtr(int channel_id) const;

  
  
  ViEEncoder* ViEEncoderPtr(int video_channel_id) const;

  
  int FreeChannelId();

  
  void ReturnChannelId(int channel_id);

  
  ChannelGroup* FindGroup(int channel_id);

  
  
  bool ChannelUsingViEEncoder(int channel_id) const;
  void ChannelsUsingViEEncoder(int channel_id, ChannelList* channels) const;

  
  CriticalSectionWrapper* channel_id_critsect_;
  int engine_id_;
  int number_of_cores_;

  
  ChannelMap channel_map_;
  bool* free_channel_ids_;
  int free_channel_ids_size_;

  
  std::list<ChannelGroup*> channel_groups_;

  
  
  EncoderMap vie_encoder_map_;
  VoEVideoSync* voice_sync_interface_;

  VoiceEngine* voice_engine_;
  ProcessThread* module_process_thread_;
  const Config& engine_config_;
  CPULoadStateCallbackInvoker* load_manager_;
};

class ViEChannelManagerScoped: private ViEManagerScopedBase {
 public:
  explicit ViEChannelManagerScoped(
      const ViEChannelManager& vie_channel_manager);
  ViEChannel* Channel(int vie_channel_id) const;
  ViEEncoder* Encoder(int vie_channel_id) const;

  
  
  bool ChannelUsingViEEncoder(int channel_id) const;

  
  
  void ChannelsUsingViEEncoder(int channel_id, ChannelList* channels) const;
};

}  

#endif  
