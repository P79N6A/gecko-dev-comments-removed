









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DELAY_MANAGER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DELAY_MANAGER_H_

#include <string.h>  

#include <vector>

#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class DelayPeakDetector;

class DelayManager {
 public:
  typedef std::vector<int> IATVector;

  
  
  
  
  DelayManager(int max_packets_in_buffer, DelayPeakDetector* peak_detector);

  virtual ~DelayManager();

  
  virtual const IATVector& iat_vector() const;

  
  
  
  
  
  virtual int Update(uint16_t sequence_number,
                     uint32_t timestamp,
                     int sample_rate_hz);

  
  
  
  
  virtual int CalculateTargetLevel(int iat_packets);

  
  
  
  virtual int SetPacketAudioLength(int length_ms);

  
  virtual void Reset();

  
  
  
  
  
  
  virtual int AverageIAT() const;

  
  
  
  virtual bool PeakFound() const;

  
  
  virtual void UpdateCounters(int elapsed_time_ms);

  
  virtual void ResetPacketIatCount();

  
  
  
  virtual void BufferLimits(int* lower_limit, int* higher_limit) const;

  
  
  virtual int TargetLevel() const;

  virtual void LastDecoderType(NetEqDecoder decoder_type);

  
  
  virtual bool SetMinimumDelay(int delay_ms);
  virtual bool SetMaximumDelay(int delay_ms);
  virtual int least_required_delay_ms() const;
  virtual int base_target_level() const;
  virtual void set_streaming_mode(bool value);
  virtual int last_pack_cng_or_dtmf() const;
  virtual void set_last_pack_cng_or_dtmf(int value);

 private:
  static const int kLimitProbability = 53687091;  
  static const int kLimitProbabilityStreaming = 536871;  
  static const int kMaxStreamingPeakPeriodMs = 600000;  
  static const int kCumulativeSumDrift = 2;  
                                             
  
  static const int kIatFactor_ = 32745;
  static const int kMaxIat = 64;  

  
  
  void ResetHistogram();

  
  
  void UpdateCumulativeSums(int packet_len_ms, uint16_t sequence_number);

  
  
  
  void UpdateHistogram(size_t iat_packets);

  
  
  
  void LimitTargetLevel();

  bool first_packet_received_;
  const int max_packets_in_buffer_;  
  IATVector iat_vector_;  
  int iat_factor_;  
  int packet_iat_count_ms_;  
  int base_target_level_;   
                            
  
  
  int target_level_;  
                      
  int packet_len_ms_;  
  bool streaming_mode_;
  uint16_t last_seq_no_;  
  uint32_t last_timestamp_;  
  int minimum_delay_ms_;  
  int least_required_delay_ms_;  
                              
                              
  int maximum_delay_ms_;  
  int iat_cumulative_sum_;  
  int max_iat_cumulative_sum_;  
  int max_timer_ms_;  
  DelayPeakDetector& peak_detector_;
  int last_pack_cng_or_dtmf_;

  DISALLOW_COPY_AND_ASSIGN(DelayManager);
};

}  
#endif  
