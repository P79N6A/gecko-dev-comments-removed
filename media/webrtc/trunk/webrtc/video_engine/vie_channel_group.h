









#ifndef WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_GROUP_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_GROUP_H_

#include <set>

#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class BitrateController;
class CallStats;
class Config;
class EncoderStateFeedback;
class ProcessThread;
class RemoteBitrateEstimator;
class ViEChannel;
class ViEEncoder;
class VieRemb;



class ChannelGroup {
 public:
  ChannelGroup(int engine_id, ProcessThread* process_thread,
               const Config* config);
  ~ChannelGroup();

  void AddChannel(int channel_id);
  void RemoveChannel(int channel_id, unsigned int ssrc);
  bool HasChannel(int channel_id);
  bool Empty();

  bool SetChannelRembStatus(int channel_id, bool sender, bool receiver,
                            ViEChannel* channel);

  BitrateController* GetBitrateController();
  CallStats* GetCallStats();
  RemoteBitrateEstimator* GetRemoteBitrateEstimator();
  EncoderStateFeedback* GetEncoderStateFeedback();

 private:
  typedef std::set<int> ChannelSet;

  scoped_ptr<VieRemb> remb_;
  scoped_ptr<BitrateController> bitrate_controller_;
  scoped_ptr<CallStats> call_stats_;
  scoped_ptr<RemoteBitrateEstimator> remote_bitrate_estimator_;
  scoped_ptr<EncoderStateFeedback> encoder_state_feedback_;
  ChannelSet channels_;
  const Config* config_;
  
  scoped_ptr<Config> own_config_;

  
  ProcessThread* process_thread_;
};

}  

#endif  
