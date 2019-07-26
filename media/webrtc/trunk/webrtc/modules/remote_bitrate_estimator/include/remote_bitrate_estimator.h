











#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_

#include <map>
#include <vector>

#include "common_types.h"
#include "typedefs.h"

namespace webrtc {



class RemoteBitrateObserver {
 public:
  
  
  virtual void OnReceiveBitrateChanged(std::vector<unsigned int>* ssrcs,
                                       unsigned int bitrate) = 0;

  virtual ~RemoteBitrateObserver() {}
};

class RemoteBitrateEstimator {
 public:
  enum EstimationMode {
    kMultiStreamEstimation,
    kSingleStreamEstimation
  };

  virtual ~RemoteBitrateEstimator() {}

  static RemoteBitrateEstimator* Create(RemoteBitrateObserver* observer,
                                        const OverUseDetectorOptions& options,
                                        EstimationMode mode);

  
  
  
  virtual void IncomingRtcp(unsigned int ssrc, uint32_t ntp_secs,
                            uint32_t ntp_frac, uint32_t rtp_timestamp) = 0;

  
  
  
  
  virtual void IncomingPacket(unsigned int ssrc,
                              int payload_size,
                              int64_t arrival_time,
                              uint32_t rtp_timestamp) = 0;

  
  virtual void UpdateEstimate(unsigned int ssrc, int64_t time_now) = 0;

  
  
  virtual void SetRtt(unsigned int rtt) = 0;

  
  virtual void RemoveStream(unsigned int ssrc) = 0;

  
  
  
  virtual bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                              unsigned int* bitrate_bps) const = 0;
};

}  

#endif
