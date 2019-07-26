











#ifndef WEBRTC_MODULES_RTP_RTCP_RTP_PACKET_HISTORY_H_
#define WEBRTC_MODULES_RTP_RTCP_RTP_PACKET_HISTORY_H_

#include <vector>

#include "module_common_types.h"
#include "rtp_rtcp_defines.h"
#include "typedefs.h"

namespace webrtc {

class RtpRtcpClock;
class CriticalSectionWrapper;

class RTPPacketHistory {
 public:
  RTPPacketHistory(RtpRtcpClock* clock);
  ~RTPPacketHistory();

  void SetStorePacketsStatus(bool enable, uint16_t number_to_store);

  bool StorePackets() const;

  
  int32_t PutRTPPacket(const uint8_t* packet,
                       uint16_t packet_length,
                       uint16_t max_packet_length,
                       int64_t capture_time_ms,
                       StorageType type);

  
  
  
  
  int32_t ReplaceRTPHeader(const uint8_t* packet,
                           uint16_t sequence_number,
                           uint16_t rtp_header_length);

  
  
  
  
  
  
  
  
  
  
  
  bool GetRTPPacket(uint16_t sequence_number,
                    uint32_t min_elapsed_time_ms,
                    uint8_t* packet,
                    uint16_t* packet_length,
                    int64_t* stored_time_ms,
                    StorageType* type) const;

  bool HasRTPPacket(uint16_t sequence_number) const;

  void UpdateResendTime(uint16_t sequence_number);

 private:
  void Allocate(uint16_t number_to_store);
  void Free();
  void VerifyAndAllocatePacketLength(uint16_t packet_length);
  bool FindSeqNum(uint16_t sequence_number, int32_t* index) const;

 private:
  RtpRtcpClock& clock_;
  CriticalSectionWrapper* critsect_;
  bool store_;
  uint32_t prev_index_;
  uint16_t max_packet_length_;

  std::vector<std::vector<uint8_t> > stored_packets_;
  std::vector<uint16_t> stored_seq_nums_;
  std::vector<uint16_t> stored_lengths_;
  std::vector<int64_t> stored_times_;
  std::vector<int64_t> stored_resend_times_;
  std::vector<StorageType> stored_types_;
};
}  
#endif  
