









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_MOCK_MOCK_REMOTE_BITRATE_ESTIMATOR_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_MOCK_MOCK_REMOTE_BITRATE_ESTIMATOR_H_

#include <gmock/gmock.h>

#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"

namespace webrtc {

class MockRemoteBitrateObserver : public RemoteBitrateObserver {
 public:
  MOCK_METHOD2(OnReceiveBitrateChanged,
      void(unsigned int ssrc, unsigned int bitrate));
};

}  

#endif  
