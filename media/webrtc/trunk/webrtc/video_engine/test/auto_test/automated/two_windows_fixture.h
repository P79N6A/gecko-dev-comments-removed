









#ifndef SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_AUTOMATED_TWO_WINDOWS_FIXTURE_H_
#define SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_AUTOMATED_TWO_WINDOWS_FIXTURE_H_

#include "gtest/gtest.h"

class ViEWindowCreator;
class ViEAutoTest;


class TwoWindowsFixture : public testing::Test {
 public:
  
  
  static void SetUpTestCase();

  
  static void TearDownTestCase();

 protected:
  static void* window_1_;
  static void* window_2_;
  static ViEWindowCreator* window_creator_;
};

#endif  
