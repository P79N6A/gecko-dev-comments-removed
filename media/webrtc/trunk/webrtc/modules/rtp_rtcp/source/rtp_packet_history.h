











#ifndef WEBRTC_MODULES_RTP_RTCP_RTP_PACKET_HISTORY_H_
#define WEBRTC_MODULES_RTP_RTCP_RTP_PACKET_HISTORY_H_

#include <vector>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Clock;
class CriticalSectionWrapper;

class RTPPacketHistory {
 public:
  RTPPacketHistory(Clock* clock);
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

  
  
  
  
  
  
  
  
  
  
  
  bool GetPacketAndSetSendTime(uint16_t sequence_number,
                               uint32_t min_elapsed_time_ms,
                               bool retransmit,
                               uint8_t* packet,
                               uint16_t* packet_length,
                               int64_t* stored_time_ms);

  bool GetBestFittingPacket(uint8_t* packet, uint16_t* packet_length,
                            int64_t* stored_time_ms);

  bool HasRTPPacket(uint16_t sequence_number) const;

 private:
  void GetPacket(int index, uint8_t* packet, uint16_t* packet_length,
                 int64_t* stored_time_ms) const;
  void Allocate(uint16_t number_to_store);
  void Free();
  void VerifyAndAllocatePacketLength(uint16_t packet_length);
  bool FindSeqNum(uint16_t sequence_number, int32_t* index) const;
  int FindBestFittingPacket(uint16_t size) const;

 private:
  Clock* clock_;
  CriticalSectionWrapper* critsect_;
  bool store_;
  uint32_t prev_index_;
  uint16_t max_packet_length_;

  std::vector<std::vector<uint8_t> > stored_packets_;
  std::vector<uint16_t> stored_seq_nums_;
  std::vector<uint16_t> stored_lengths_;
  std::vector<int64_t> stored_times_;
  std::vector<int64_t> stored_send_times_;
  std::vector<StorageType> stored_types_;
};
}  
#endif  
