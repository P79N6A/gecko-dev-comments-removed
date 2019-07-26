













#ifndef WEBRTC_VOICE_ENGINE_VOE_TEST_INTERFACE_H
#define WEBRTC_VOICE_ENGINE_VOE_TEST_INTERFACE_H

#include "webrtc/common_types.h"

namespace voetest {

using namespace webrtc;


enum TestType {
  Invalid = -1,
  Standard = 0,

  Stress = 2,
  Unit = 3,
  CPU = 4
};


int runAutoTest(TestType testType);

}  
#endif 
