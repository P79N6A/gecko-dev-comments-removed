









#ifndef SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_HELPERS_VIE_WINDOW_CREATOR_H_
#define SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_HELPERS_VIE_WINDOW_CREATOR_H_

class ViEAutoTestWindowManagerInterface;

class ViEWindowCreator {
 public:
  ViEWindowCreator();
  virtual ~ViEWindowCreator();

  
  
  ViEAutoTestWindowManagerInterface* CreateTwoWindows();

  
  
  void TerminateWindows();
 private:
  ViEAutoTestWindowManagerInterface* window_manager_;
};

#endif  
