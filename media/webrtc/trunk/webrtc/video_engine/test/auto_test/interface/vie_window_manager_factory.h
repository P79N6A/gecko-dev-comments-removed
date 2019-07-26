









#ifndef SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_WINDOW_MANAGER_FACTORY_H_
#define SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_WINDOW_MANAGER_FACTORY_H_

class ViEAutoTestWindowManagerInterface;

class ViEWindowManagerFactory {
 public:
  
  
  
  static ViEAutoTestWindowManagerInterface*
  CreateWindowManagerForCurrentPlatform();
};

#endif  
