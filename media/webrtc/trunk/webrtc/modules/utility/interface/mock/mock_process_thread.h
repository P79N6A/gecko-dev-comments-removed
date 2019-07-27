









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_MOCK_PROCESS_THREAD_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_MOCK_PROCESS_THREAD_H_

#include "webrtc/modules/utility/interface/process_thread.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace webrtc {

class MockProcessThread : public ProcessThread {
 public:
  MOCK_METHOD0(Start, int32_t());
  MOCK_METHOD0(Stop, int32_t());
  MOCK_METHOD1(RegisterModule, int32_t(Module* module));
  MOCK_METHOD1(DeRegisterModule, int32_t(const Module* module));
};

}  
#endif  
