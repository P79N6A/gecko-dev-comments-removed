









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_DTMF_QUEUE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_DTMF_QUEUE_H_

#include "webrtc/modules/rtp_rtcp/source/rtp_rtcp_config.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class DTMFqueue {
 public:
  DTMFqueue();
  virtual ~DTMFqueue();

  int32_t AddDTMF(uint8_t dtmf_key, uint16_t len, uint8_t level);
  int8_t NextDTMF(uint8_t* dtmf_key, uint16_t* len, uint8_t* level);
  bool PendingDTMF();
  void ResetDTMF();

 private:
  CriticalSectionWrapper* dtmf_critsect_;
  uint8_t next_empty_index_;
  uint8_t dtmf_key_[DTMF_OUTBAND_MAX];
  uint16_t dtmf_length[DTMF_OUTBAND_MAX];
  uint8_t dtmf_level_[DTMF_OUTBAND_MAX];
};
}  

#endif  
