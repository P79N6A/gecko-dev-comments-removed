









#include "webrtc/video_engine/test/auto_test/interface/vie_window_manager_factory.h"

#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_linux.h"

ViEAutoTestWindowManagerInterface*
ViEWindowManagerFactory::CreateWindowManagerForCurrentPlatform() {
  return new ViEAutoTestWindowManager();
}
