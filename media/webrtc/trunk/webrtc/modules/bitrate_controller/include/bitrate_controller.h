













#ifndef WEBRTC_MODULES_BITRATE_CONTROLLER_INCLUDE_BITRATE_CONTROLLER_H_
#define WEBRTC_MODULES_BITRATE_CONTROLLER_INCLUDE_BITRATE_CONTROLLER_H_

#include "modules/rtp_rtcp/interface/rtp_rtcp_defines.h"

namespace webrtc {

class BitrateObserver {
 






 public:
  virtual void OnNetworkChanged(const uint32_t target_bitrate,
                                const uint8_t fraction_loss,  
                                const uint32_t rtt) = 0;

  virtual ~BitrateObserver() {}
};

class BitrateController {






 public:
  static BitrateController* CreateBitrateController();
  virtual ~BitrateController() {}

  virtual RtcpBandwidthObserver* CreateRtcpBandwidthObserver() = 0;

  
  
  virtual bool AvailableBandwidth(uint32_t* bandwidth) const = 0;

  






  virtual void SetBitrateObserver(BitrateObserver* observer,
                                  const uint32_t start_bitrate,
                                  const uint32_t min_bitrate,
                                  const uint32_t max_bitrate) = 0;

  virtual void RemoveBitrateObserver(BitrateObserver* observer) = 0;
};
}  
#endif  
