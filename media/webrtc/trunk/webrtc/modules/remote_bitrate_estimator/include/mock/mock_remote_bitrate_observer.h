









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_MOCK_MOCK_REMOTE_BITRATE_ESTIMATOR_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_MOCK_MOCK_REMOTE_BITRATE_ESTIMATOR_H_

#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"

namespace webrtc {

class MockRemoteBitrateObserver : public RemoteBitrateObserver {
 public:
  MOCK_METHOD2(OnReceiveBitrateChanged,
      void(const std::vector<unsigned int>& ssrcs, unsigned int bitrate));
};

}  

#endif  
