









#ifndef WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_GROUP_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_GROUP_H_

#include <set>

#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class ProcessThread;
class ViEChannel;
class ViEEncoder;
class VieRemb;



class ChannelGroup {
 public:
  explicit ChannelGroup(ProcessThread* process_thread);
  ~ChannelGroup();

  void AddChannel(int channel_id);
  void RemoveChannel(int channel_id);
  bool HasChannel(int channel_id);
  bool Empty();

  bool SetChannelRembStatus(int channel_id,
                            bool sender,
                            bool receiver,
                            ViEChannel* channel,
                            ViEEncoder* encoder);

 private:
  typedef std::set<int> ChannelSet;

  scoped_ptr<VieRemb> remb_;
  ChannelSet channels_;
};

}  

#endif  
