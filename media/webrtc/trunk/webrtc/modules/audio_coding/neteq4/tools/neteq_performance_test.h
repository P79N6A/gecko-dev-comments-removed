









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TOOLS_NETEQ_PERFORMANCE_TEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TOOLS_NETEQ_PERFORMANCE_TEST_H_

#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class NetEqPerformanceTest {
 public:
  
  
  
  
  
  static int64_t Run(int runtime_ms, int lossrate, double drift_factor);
};

}  
}  

#endif  
