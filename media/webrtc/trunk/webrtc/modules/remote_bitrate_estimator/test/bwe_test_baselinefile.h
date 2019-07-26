









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_BASELINEFILE_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TEST_BWE_TEST_BASELINEFILE_H_

#include <string>
#include "webrtc/modules/interface/module_common_types.h"

namespace webrtc {
namespace testing {
namespace bwe {

class BaseLineFileInterface {
 public:
  virtual ~BaseLineFileInterface() {}

  
  virtual void Estimate(int64_t time_ms, uint32_t estimate_bps) = 0;

  
  
  
  
  virtual bool VerifyOrWrite() = 0;

  
  
  
  
  static BaseLineFileInterface* Create(const std::string& filename,
                                       bool write_updated_file);
};
}  
}  
}  

#endif  
