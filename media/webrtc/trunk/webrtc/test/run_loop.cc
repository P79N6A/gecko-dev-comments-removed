








#include "webrtc/test/run_loop.h"

#include <stdio.h>

namespace webrtc {
namespace test {

void PressEnterToContinue() {
  puts(">> Press ENTER to continue...");
  while (getc(stdin) != '\n' && !feof(stdin));
}
}  
}  
