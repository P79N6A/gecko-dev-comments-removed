









#ifndef WEBRTC_VIDEO_ENGINE_VIE_REMB_H_
#define WEBRTC_VIDEO_ENGINE_VIE_REMB_H_

#include <list>
#include <utility>
#include <vector>

#include "modules/interface/module.h"
#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CriticalSectionWrapper;
class ProcessThread;
class RtpRtcp;

class VieRemb : public RemoteBitrateObserver, public Module {
 public:
  explicit VieRemb(ProcessThread* process_thread);
  ~VieRemb();

  
  void AddReceiveChannel(RtpRtcp* rtp_rtcp);

  
  void RemoveReceiveChannel(RtpRtcp* rtp_rtcp);

  
  void AddRembSender(RtpRtcp* rtp_rtcp);

  
  void RemoveRembSender(RtpRtcp* rtp_rtcp);

  
  bool InUse() const;

  
  
  
  
  
  virtual void OnReceiveBitrateChanged(std::vector<unsigned int>* ssrcs,
                                       unsigned int bitrate);

  
  virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);
  virtual WebRtc_Word32 TimeUntilNextProcess();
  virtual WebRtc_Word32 Process();

 private:
  typedef std::list<RtpRtcp*> RtpModules;

  ProcessThread* process_thread_;
  scoped_ptr<CriticalSectionWrapper> list_crit_;

  
  int64_t last_remb_time_;
  unsigned int last_send_bitrate_;

  
  RtpModules receive_modules_;

  
  RtpModules rtcp_sender_;

  
  unsigned int bitrate_;
  std::vector<unsigned int> ssrcs_;
  int64_t bitrate_update_time_ms_;
};

}  

#endif  
