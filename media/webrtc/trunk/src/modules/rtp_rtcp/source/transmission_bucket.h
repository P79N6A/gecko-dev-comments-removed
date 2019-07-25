









#ifndef WEBRTC_MODULES_RTP_RTCP_TRANSMISSION_BUCKET_H_
#define WEBRTC_MODULES_RTP_RTCP_TRANSMISSION_BUCKET_H_

#include <vector>

#include "typedefs.h"

namespace webrtc
{
class CriticalSectionWrapper;

class TransmissionBucket {
 public:
  TransmissionBucket();
  ~TransmissionBucket();

  
  void Reset();

  
  void Fill(const uint16_t seq_num, const uint32_t num_bytes);

  
  bool Empty();

  
  void UpdateBytesPerInterval(const uint32_t delta_time_in_ms,
                              const uint16_t target_bitrate_kbps);

  
  
  
  int32_t GetNextPacket();

 private:
   struct Packet {
     Packet(uint16_t sequence_number, uint16_t length_in_bytes)
       : sequence_number_(sequence_number),
         length_(length_in_bytes) {
     }
     uint16_t sequence_number_;
     uint16_t length_;
   };

   CriticalSectionWrapper* critsect_;
   uint32_t accumulator_;
   int32_t bytes_rem_total_;
   int32_t bytes_rem_interval_;
   std::vector<Packet> packets_;
   bool first_;
};
}  
#endif  
