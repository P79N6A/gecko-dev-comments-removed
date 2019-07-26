











#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_

#include <map>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Clock;



class RemoteBitrateObserver {
 public:
  
  
  virtual void OnReceiveBitrateChanged(std::vector<unsigned int>* ssrcs,
                                       unsigned int bitrate) = 0;

  virtual ~RemoteBitrateObserver() {}
};

class RemoteBitrateEstimator : public CallStatsObserver, public Module {
 public:
  enum EstimationMode {
    kMultiStreamEstimation,
    kSingleStreamEstimation
  };

  virtual ~RemoteBitrateEstimator() {}

  static RemoteBitrateEstimator* Create(const OverUseDetectorOptions& options,
                                        EstimationMode mode,
                                        RemoteBitrateObserver* observer,
                                        Clock* clock);

  
  
  
  virtual void IncomingRtcp(unsigned int ssrc, uint32_t ntp_secs,
                            uint32_t ntp_frac, uint32_t rtp_timestamp) = 0;

  
  
  
  
  virtual void IncomingPacket(unsigned int ssrc,
                              int payload_size,
                              int64_t arrival_time,
                              uint32_t rtp_timestamp) = 0;

  
  virtual void RemoveStream(unsigned int ssrc) = 0;

  
  
  
  virtual bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                              unsigned int* bitrate_bps) const = 0;

 protected:
  static const int kProcessIntervalMs = 1000;
  static const int kStreamTimeOutMs = 2000;
};

}  

#endif
