












#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_

#include <map>

#include "modules/remote_bitrate_estimator/bitrate_estimator.h"
#include "modules/remote_bitrate_estimator/overuse_detector.h"
#include "modules/remote_bitrate_estimator/remote_rate_control.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"

namespace webrtc {



class RemoteBitrateObserver {
 public:
  
  
  virtual void OnReceiveBitrateChanged(unsigned int ssrc,
                                       unsigned int bitrate) = 0;

  virtual ~RemoteBitrateObserver() {}
};

class RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimator(RemoteBitrateObserver* observer,
                         const OverUseDetectorOptions& options);

  
  
  void IncomingPacket(unsigned int ssrc,
                      int packet_size,
                      int64_t arrival_time,
                      uint32_t rtp_timestamp,
                      int64_t packet_send_time);

  
  void UpdateEstimate(unsigned int ssrc, int64_t time_now);

  
  
  void SetRtt(unsigned int ssrc);

  
  void RemoveStream(unsigned int ssrc);

  
  
  bool LatestEstimate(unsigned int ssrc, unsigned int* bitrate_bps) const;

 private:
  struct BitrateControls {
    explicit BitrateControls(const OverUseDetectorOptions& options)
        : remote_rate(),
          overuse_detector(options),
          incoming_bitrate() {
    }
    BitrateControls(const BitrateControls& other)
        : remote_rate(other.remote_rate),
          overuse_detector(other.overuse_detector),
          incoming_bitrate(other.incoming_bitrate) {
    }
    RemoteRateControl remote_rate;
    OverUseDetector overuse_detector;
    BitRateStats incoming_bitrate;
  };

  typedef std::map<unsigned int, BitrateControls> SsrcBitrateControlsMap;

  const OverUseDetectorOptions& options_;
  SsrcBitrateControlsMap bitrate_controls_;
  RemoteBitrateObserver* observer_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
};

}  

#endif  
