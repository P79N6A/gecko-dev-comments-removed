









#include "video_engine/test/auto_test/interface/vie_window_creator.h"

#include "video_engine/include/vie_codec.h"
#include "video_engine/test/auto_test/interface/vie_autotest_main.h"
#include "video_engine/test/auto_test/interface/vie_autotest_window_manager_interface.h"
#include "video_engine/test/auto_test/interface/vie_window_manager_factory.h"
#include "voice_engine/include/voe_codec.h"

#if defined(WIN32)
#include <tchar.h>
#endif

ViEWindowCreator::ViEWindowCreator() {
#ifndef WEBRTC_ANDROID
  window_manager_ =
      ViEWindowManagerFactory::CreateWindowManagerForCurrentPlatform();
#endif
}

ViEWindowCreator::~ViEWindowCreator() {
  delete window_manager_;
}

ViEAutoTestWindowManagerInterface*
  ViEWindowCreator::CreateTwoWindows() {
#if defined(WIN32)
  TCHAR window1Title[1024] = _T("ViE Autotest Window 1");
  TCHAR window2Title[1024] = _T("ViE Autotest Window 2");
#else
  char window1Title[1024] = "ViE Autotest Window 1";
  char window2Title[1024] = "ViE Autotest Window 2";
#endif

  AutoTestRect window1Size(352, 288, 600, 100);
  AutoTestRect window2Size(352, 288, 1000, 100);
  window_manager_->CreateWindows(window1Size, window2Size, window1Title,
                                 window2Title);
  window_manager_->SetTopmostWindow();

  return window_manager_;
}

void ViEWindowCreator::TerminateWindows() {
  window_manager_->TerminateWindows();
}
