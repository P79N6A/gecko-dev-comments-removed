











#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_H_

#include <map>

#include "webrtc/modules/remote_bitrate_estimator/bitrate_estimator.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/remote_bitrate_estimator/overuse_detector.h"
#include "webrtc/modules/remote_bitrate_estimator/remote_rate_control.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Clock;

class RemoteBitrateEstimatorSingleStream : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorSingleStream(const OverUseDetectorOptions& options,
                                     RemoteBitrateObserver* observer,
                                     Clock* clock);

  virtual ~RemoteBitrateEstimatorSingleStream() {}

  void IncomingRtcp(unsigned int ssrc, uint32_t ntp_secs, uint32_t ntp_frac,
                    uint32_t rtp_timestamp) {}

  
  
  
  
  
  void IncomingPacket(unsigned int ssrc,
                      int payload_size,
                      int64_t arrival_time,
                      uint32_t rtp_timestamp);

  
  
  virtual int32_t Process();
  virtual int32_t TimeUntilNextProcess();
  
  
  virtual void OnRttUpdate(uint32_t rtt);

  
  void RemoveStream(unsigned int ssrc);

  
  
  
  bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                      unsigned int* bitrate_bps) const;

 private:
  typedef std::map<unsigned int, OveruseDetector> SsrcOveruseDetectorMap;

  
  void UpdateEstimate(int64_t time_now);

  void GetSsrcs(std::vector<unsigned int>* ssrcs) const;

  const OverUseDetectorOptions& options_;
  Clock* clock_;
  SsrcOveruseDetectorMap overuse_detectors_;
  BitRateStats incoming_bitrate_;
  RemoteRateControl remote_rate_;
  RemoteBitrateObserver* observer_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  int64_t last_process_time_;
};

}  

#endif  
