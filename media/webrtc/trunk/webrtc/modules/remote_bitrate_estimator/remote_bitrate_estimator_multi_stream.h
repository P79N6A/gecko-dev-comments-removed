











#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_IMPL_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_IMPL_H_

#include <map>

#include "modules/remote_bitrate_estimator/bitrate_estimator.h"
#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "modules/remote_bitrate_estimator/include/rtp_to_ntp.h"
#include "modules/remote_bitrate_estimator/overuse_detector.h"
#include "modules/remote_bitrate_estimator/remote_rate_control.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"

namespace webrtc {

class Clock;

class RemoteBitrateEstimatorMultiStream : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorMultiStream(const OverUseDetectorOptions& options,
                                    RemoteBitrateObserver* observer,
                                    Clock* clock);

  ~RemoteBitrateEstimatorMultiStream() {}

  
  
  
  void IncomingRtcp(unsigned int ssrc, uint32_t ntp_secs, uint32_t ntp_frac,
                    uint32_t rtp_timestamp);

  
  
  
  
  
  
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
  typedef std::map<unsigned int, synchronization::RtcpList> StreamMap;

  
  void UpdateEstimate(int64_t time_now);

  void GetSsrcs(std::vector<unsigned int>* ssrcs) const;

  Clock* clock_;
  RemoteRateControl remote_rate_;
  OveruseDetector overuse_detector_;
  BitRateStats incoming_bitrate_;
  RemoteBitrateObserver* observer_;
  StreamMap streams_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  unsigned int initial_ssrc_;
  bool multi_stream_;
  int32_t last_process_time_;

  DISALLOW_COPY_AND_ASSIGN(RemoteBitrateEstimatorMultiStream);
};

}  

#endif  
