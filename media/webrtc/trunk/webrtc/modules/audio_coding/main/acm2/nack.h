









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_NACK_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_NACK_H_

#include <vector>
#include <map>

#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"






























namespace webrtc {

namespace acm2 {

class Nack {
 public:
  
  static const size_t kNackListSizeLimit = 500;  
                                                 
  
  static Nack* Create(int nack_threshold_packets);

  ~Nack() {}

  
  
  
  
  
  int SetMaxNackListSize(size_t max_nack_list_size);

  
  
  
  
  
  
  void UpdateSampleRate(int sample_rate_hz);

  
  
  void UpdateLastDecodedPacket(uint16_t sequence_number, uint32_t timestamp);

  
  
  void UpdateLastReceivedPacket(uint16_t sequence_number, uint32_t timestamp);

  
  
  
  std::vector<uint16_t> GetNackList(int round_trip_time_ms) const;

  
  
  void Reset();

 private:
  
  FRIEND_TEST_ALL_PREFIXES(NackTest, EstimateTimestampAndTimeToPlay);

  struct NackElement {
    NackElement(int initial_time_to_play_ms,
                uint32_t initial_timestamp,
                bool missing)
        : time_to_play_ms(initial_time_to_play_ms),
          estimated_timestamp(initial_timestamp),
          is_missing(missing) {}

    
    
    int time_to_play_ms;

    
    
    
    
    
    
    uint32_t estimated_timestamp;

    
    
    bool is_missing;
  };

  class NackListCompare {
   public:
    bool operator() (uint16_t sequence_number_old,
                     uint16_t sequence_number_new) const {
      return IsNewerSequenceNumber(sequence_number_new, sequence_number_old);
    }
  };

  typedef std::map<uint16_t, NackElement, NackListCompare> NackList;

  
  explicit Nack(int nack_threshold_packets);

  
  
  NackList GetNackList() const;

  
  
  void AddToList(uint16_t sequence_number_current_received_rtp);

  
  
  void UpdateEstimatedPlayoutTimeBy10ms();

  
  
  
  void UpdateSamplesPerPacket(uint16_t sequence_number_current_received_rtp,
                              uint32_t timestamp_current_received_rtp);

  
  
  
  void UpdateList(uint16_t sequence_number_current_received_rtp);

  
  
  void ChangeFromLateToMissing(uint16_t sequence_number_current_received_rtp);

  
  
  
  void LimitNackListSize();

  
  uint32_t EstimateTimestamp(uint16_t sequence_number);

  
  int TimeToPlay(uint32_t timestamp) const;

  
  
  
  
  
  const int nack_threshold_packets_;

  
  uint16_t sequence_num_last_received_rtp_;
  uint32_t timestamp_last_received_rtp_;
  bool any_rtp_received_;  

  
  uint16_t sequence_num_last_decoded_rtp_;
  uint32_t timestamp_last_decoded_rtp_;
  bool any_rtp_decoded_;  

  int sample_rate_khz_;  

  
  
  int samples_per_packet_;

  
  
  
  NackList nack_list_;

  
  
  size_t max_nack_list_size_;
};

}  

}  

#endif  
