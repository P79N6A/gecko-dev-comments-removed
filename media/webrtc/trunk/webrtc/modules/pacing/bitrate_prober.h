









#ifndef WEBRTC_MODULES_PACING_BITRATE_PROBER_H_
#define WEBRTC_MODULES_PACING_BITRATE_PROBER_H_

#include <cstddef>
#include <list>

#include "webrtc/typedefs.h"

namespace webrtc {



class BitrateProber {
 public:
  BitrateProber();

  void SetEnabled(bool enable);

  
  
  
  bool IsProbing() const;

  
  void MaybeInitializeProbe(int bitrate_bps);

  
  
  int TimeUntilNextProbe(int64_t now_ms);

  
  
  void PacketSent(int64_t now_ms, size_t packet_size);

 private:
  enum ProbingState { kDisabled, kAllowedToProbe, kProbing, kWait };

  ProbingState probing_state_;
  
  
  
  std::list<int> probe_bitrates_;
  size_t packet_size_last_send_;
  int64_t time_last_send_ms_;
};
}  
#endif
