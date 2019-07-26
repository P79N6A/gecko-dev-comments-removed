









#ifndef WEBRTC_EXPERIMENTS_H_
#define WEBRTC_EXPERIMENTS_H_

namespace webrtc {
struct PaddingStrategy {
  PaddingStrategy()
      : redundant_payloads(false) {}
  explicit PaddingStrategy(bool redundant_payloads)
      : redundant_payloads(redundant_payloads) {}
  virtual ~PaddingStrategy() {}

  const bool redundant_payloads;
};

struct RemoteBitrateEstimatorMinRate {
  RemoteBitrateEstimatorMinRate() : min_rate(30000) {}
  RemoteBitrateEstimatorMinRate(uint32_t min_rate) : min_rate(min_rate) {}

  uint32_t min_rate;
};

struct SkipEncodingUnusedStreams {
  SkipEncodingUnusedStreams() : enabled(false) {}
  explicit SkipEncodingUnusedStreams(bool set_enabled)
    : enabled(set_enabled) {}
  virtual ~SkipEncodingUnusedStreams() {}

  const bool enabled;
};
}  
#endif  
