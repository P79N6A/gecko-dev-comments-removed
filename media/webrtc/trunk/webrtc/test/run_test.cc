









#include "webrtc/test/run_test.h"

#include <stdio.h>

namespace webrtc {
namespace test {

void RunTest(void(*test)()) {
  (*test)();
}

}  
}  
