











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
  
  
  virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs,
                                       unsigned int bitrate) = 0;

  virtual ~RemoteBitrateObserver() {}
};

class RemoteBitrateEstimator : public CallStatsObserver, public Module {
 public:
  virtual ~RemoteBitrateEstimator() {}

  
  
  
  
  virtual void IncomingPacket(int64_t arrival_time_ms,
                              int payload_size,
                              const RTPHeader& header) = 0;

  
  virtual void RemoveStream(unsigned int ssrc) = 0;

  
  
  
  virtual bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                              unsigned int* bitrate_bps) const = 0;

 protected:
  static const int kProcessIntervalMs = 1000;
  static const int kStreamTimeOutMs = 2000;
};

struct RemoteBitrateEstimatorFactory {
  RemoteBitrateEstimatorFactory() {}
  virtual ~RemoteBitrateEstimatorFactory() {}

  virtual RemoteBitrateEstimator* Create(
      RemoteBitrateObserver* observer,
      Clock* clock) const;
};

struct AbsoluteSendTimeRemoteBitrateEstimatorFactory {
  AbsoluteSendTimeRemoteBitrateEstimatorFactory() {}
  virtual ~AbsoluteSendTimeRemoteBitrateEstimatorFactory() {}

  virtual RemoteBitrateEstimator* Create(
      RemoteBitrateObserver* observer,
      Clock* clock) const;
};
}  

#endif  
