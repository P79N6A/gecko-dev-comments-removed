









#ifndef WEBRTC_VOICE_ENGINE_VOE_CPU_TEST_H
#define WEBRTC_VOICE_ENGINE_VOE_CPU_TEST_H

#include "voe_standard_test.h"

namespace voetest {

class VoETestManager;

class VoECpuTest {
 public:
  VoECpuTest(VoETestManager& mgr);
  ~VoECpuTest() {}
  int DoTest();
 private:
  VoETestManager& _mgr;
};

} 

#endif 
