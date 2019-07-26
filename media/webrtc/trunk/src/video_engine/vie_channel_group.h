









#ifndef WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_GROUP_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_GROUP_H_

#include <set>

#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class BitrateController;
struct OverUseDetectorOptions;
class ProcessThread;
class RemoteBitrateEstimator;
class RemoteBitrateObserver;
class ViEChannel;
class ViEEncoder;
class VieRemb;



class ChannelGroup {
 public:
  ChannelGroup(ProcessThread* process_thread,
               const OverUseDetectorOptions& options);
  ~ChannelGroup();

  void AddChannel(int channel_id);
  void RemoveChannel(int channel_id, unsigned int ssrc);
  bool HasChannel(int channel_id);
  bool Empty();

  bool SetChannelRembStatus(int channel_id,
                            bool sender,
                            bool receiver,
                            ViEChannel* channel,
                            ViEEncoder* encoder);

  BitrateController* GetBitrateController();
  RemoteBitrateEstimator* GetRemoteBitrateEstimator();

 private:
  typedef std::set<int> ChannelSet;

  scoped_ptr<VieRemb> remb_;
  scoped_ptr<BitrateController> bitrate_controller_;
  scoped_ptr<RemoteBitrateEstimator> remote_bitrate_estimator_;
  ChannelSet channels_;
};

}  

#endif  
