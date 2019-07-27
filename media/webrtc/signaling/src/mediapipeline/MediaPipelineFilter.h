








#ifndef mediapipelinefilter_h__
#define mediapipelinefilter_h__

#include <cstddef>
#include <stdint.h>

#include <set>

namespace webrtc {
struct RTPHeader;
}

namespace mozilla {























class MediaPipelineFilter {
 public:
  MediaPipelineFilter();

  
  
  
  bool Filter(const webrtc::RTPHeader& header, uint32_t correlator = 0);

  typedef enum {
    FAIL,
    PASS,
    UNSUPPORTED
  } Result;

  
  
  Result FilterRTCP(const unsigned char* data, size_t len) const;

  void AddLocalSSRC(uint32_t ssrc);
  void AddRemoteSSRC(uint32_t ssrc);

  
  void AddUniquePT(uint8_t payload_type);
  void SetCorrelator(uint32_t correlator);

  void Update(const MediaPipelineFilter& filter_update);

  
  static const uint8_t SENDER_REPORT_T = 200;
  static const uint8_t RECEIVER_REPORT_T = 201;

 private:
  static const uint8_t MAYBE_LOCAL_SSRC = 1;
  static const uint8_t MAYBE_REMOTE_SSRC = 2;

  
  static const size_t PT_OFFSET = 1;
  
  static const size_t FIRST_SSRC_OFFSET = 4;

  static const size_t RECEIVER_REPORT_START_SR = 7*4;
  static const size_t SENDER_REPORT_START_RR = 2*4;
  static const size_t RECEIVER_REPORT_SIZE = 6*4;

  bool CheckRtcpSsrc(const unsigned char* data,
                     size_t len,
                     size_t ssrc_offset,
                     uint8_t flags) const;


  bool CheckRtcpReport(const unsigned char* data,
                        size_t len,
                        size_t first_rr_offset) const;

  uint32_t correlator_;
  
  
  std::set<uint32_t> remote_ssrc_set_;
  std::set<uint32_t> local_ssrc_set_;
  std::set<uint8_t> payload_type_set_;
};

} 

#endif 

