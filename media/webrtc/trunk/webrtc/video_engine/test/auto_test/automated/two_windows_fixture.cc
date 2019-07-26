









#include "video_engine/test/auto_test/automated/two_windows_fixture.h"

#include "video_engine/test/auto_test/interface/vie_window_creator.h"
#include "video_engine/test/auto_test/interface/vie_autotest_window_manager_interface.h"

void TwoWindowsFixture::SetUpTestCase() {
  window_creator_ = new ViEWindowCreator();

  ViEAutoTestWindowManagerInterface* window_manager =
      window_creator_->CreateTwoWindows();

  window_1_ = window_manager->GetWindow1();
  window_2_ = window_manager->GetWindow2();
}

void TwoWindowsFixture::TearDownTestCase() {
  window_creator_->TerminateWindows();
  delete window_creator_;
}

ViEWindowCreator* TwoWindowsFixture::window_creator_ = NULL;
void* TwoWindowsFixture::window_1_ = NULL;
void* TwoWindowsFixture::window_2_ = NULL;
