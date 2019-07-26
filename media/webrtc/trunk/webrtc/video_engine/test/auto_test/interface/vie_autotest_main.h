









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_MAIN_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_MAIN_H_

#include <string>
#include <map>

class ViEAutoTestMain {
 public:
  ViEAutoTestMain();

  
  
  
  int RunTests(int argc, char** argv);

 private:
  std::map<int, std::string> index_to_test_method_map_;

  static const int kInvalidChoice = -1;

  
  int RunInteractiveMode();
  
  
  int RunSpecificTestCaseIn(const std::string test_case_name);
  
  int AskUserForTestCase();
  
  
  int AskUserForNumber(int min_allowed, int max_allowed);
  
  
  int RunTestMatching(const std::string test_case,
                      const std::string test_method);
  
  int RunSpecialTestCase(int choice);
};

#endif  
