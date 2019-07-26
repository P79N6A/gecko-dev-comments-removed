








#ifndef mediapipelinefilter_h__
#define mediapipelinefilter_h__

#include <cstddef>
#include <stdint.h>

#include <set>

namespace webrtc {
class RTPHeader;
}

namespace mozilla {























class MediaPipelineFilter {
 public:
  MediaPipelineFilter();

  
  
  
  bool Filter(const webrtc::RTPHeader& header, uint32_t correlator = 0);

  
  
  
  bool FilterRTCPReceiverReport(uint32_t ssrc);
  
  bool FilterRTCP(uint32_t ssrc);

  void AddLocalSSRC(uint32_t ssrc);
  void AddRemoteSSRC(uint32_t ssrc);

  
  void AddUniquePT(uint8_t payload_type);
  void SetCorrelator(uint32_t correlator);

  void IncorporateRemoteDescription(const MediaPipelineFilter& remote_filter);

 private:
  uint32_t correlator_;
  
  
  std::set<uint32_t> remote_ssrc_set_;
  std::set<uint32_t> local_ssrc_set_;
  std::set<uint8_t> payload_type_set_;
};

} 

#endif 

