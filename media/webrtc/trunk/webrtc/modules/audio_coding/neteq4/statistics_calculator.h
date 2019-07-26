









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_STATISTICS_CALCULATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_STATISTICS_CALCULATOR_H_

#include <vector>

#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class DecisionLogic;
class DelayManager;


class StatisticsCalculator {
 public:
  StatisticsCalculator();

  virtual ~StatisticsCalculator() {}

  
  void Reset();

  
  void ResetMcu();

  
  void ResetWaitingTimeStatistics();

  
  
  void ExpandedVoiceSamples(int num_samples);

  
  
  void ExpandedNoiseSamples(int num_samples);

  
  
  void PreemptiveExpandedSamples(int num_samples);

  
  void AcceleratedSamples(int num_samples);

  
  void AddZeros(int num_samples);

  
  void PacketsDiscarded(int num_packets);

  
  void LostSamples(int num_samples);

  
  
  void IncreaseCounter(int num_samples, int fs_hz);

  
  void StoreWaitingTime(int waiting_time_ms);

  
  
  
  
  void GetNetworkStatistics(int fs_hz,
                            int num_samples_in_buffers,
                            int samples_per_packet,
                            const DelayManager& delay_manager,
                            const DecisionLogic& decision_logic,
                            NetEqNetworkStatistics *stats);

  void WaitingTimes(std::vector<int>* waiting_times);

 private:
  static const int kMaxReportPeriod = 60;  
  static const int kLenWaitingTimes = 100;

  
  static int CalculateQ14Ratio(uint32_t numerator, uint32_t denominator);

  uint32_t preemptive_samples_;
  uint32_t accelerate_samples_;
  int added_zero_samples_;
  uint32_t expanded_voice_samples_;
  uint32_t expanded_noise_samples_;
  int discarded_packets_;
  uint32_t lost_timestamps_;
  uint32_t last_report_timestamp_;
  int waiting_times_[kLenWaitingTimes];  
  int len_waiting_times_;
  int next_waiting_time_index_;

  DISALLOW_COPY_AND_ASSIGN(StatisticsCalculator);
};

}  
#endif  
