








#include "webrtc/video_engine/test/common/run_loop.h"

#include <assert.h>

#include <conio.h>
#include <stdio.h>
#include <Windows.h>

namespace webrtc {
namespace test {

void PressEnterToContinue() {
  puts(">> Press ENTER to continue...");

  MSG msg;
  BOOL ret;
  while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
    assert(ret != -1);
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}
}  
}  
