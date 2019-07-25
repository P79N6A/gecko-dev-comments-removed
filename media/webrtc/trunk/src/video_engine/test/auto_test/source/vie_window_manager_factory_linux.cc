









#include "vie_window_manager_factory.h"

#include "vie_autotest_linux.h"

ViEAutoTestWindowManagerInterface*
ViEWindowManagerFactory::CreateWindowManagerForCurrentPlatform() {
  return new ViEAutoTestWindowManager();
}
