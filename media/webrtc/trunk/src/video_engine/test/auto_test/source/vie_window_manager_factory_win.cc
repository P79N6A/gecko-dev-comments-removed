








#include "vie_window_manager_factory.h"

#include "vie_autotest_windows.h"

ViEAutoTestWindowManagerInterface*
ViEWindowManagerFactory::CreateWindowManagerForCurrentPlatform() {
  return new ViEAutoTestWindowManager();
}
