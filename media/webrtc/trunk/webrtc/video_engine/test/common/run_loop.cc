








#include "webrtc/video_engine/test/common/run_loop.h"

#include <stdio.h>

namespace webrtc {
namespace test {

void PressEnterToContinue() {
  puts(">> Press ENTER to continue...");
  while (getchar() != '\n' && !feof(stdin));
}
}  
}  
