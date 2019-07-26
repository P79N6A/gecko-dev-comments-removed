









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include "gtest/gtest.h"
#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class WindowCapturerTest : public testing::Test,
                           public DesktopCapturer::Callback {
 public:
  void SetUp() OVERRIDE {
    capturer_.reset(
        WindowCapturer::Create(DesktopCaptureOptions::CreateDefault()));
  }

  void TearDown() OVERRIDE {
  }

  
  virtual SharedMemory* CreateSharedMemory(size_t size) OVERRIDE {
    return NULL;
  }

  virtual void OnCaptureCompleted(DesktopFrame* frame) OVERRIDE {
    frame_.reset(frame);
  }

 protected:
  scoped_ptr<WindowCapturer> capturer_;
  scoped_ptr<DesktopFrame> frame_;
};


TEST_F(WindowCapturerTest, Enumerate) {
  WindowCapturer::WindowList windows;
  EXPECT_TRUE(capturer_->GetWindowList(&windows));

  
  for (WindowCapturer::WindowList::iterator it = windows.begin();
       it != windows.end(); ++it) {
    EXPECT_FALSE(it->title.empty());
  }
}








TEST_F(WindowCapturerTest, Capture) {
  WindowCapturer::WindowList windows;
  capturer_->Start(this);
  EXPECT_TRUE(capturer_->GetWindowList(&windows));

  
  for (WindowCapturer::WindowList::iterator it = windows.begin();
       it != windows.end(); ++it) {
    frame_.reset();
    if (capturer_->SelectWindow(it->id)) {
      capturer_->Capture(DesktopRegion());
    }

    
    if (!frame_.get()) {
      WindowCapturer::WindowList new_list;
      EXPECT_TRUE(capturer_->GetWindowList(&new_list));
      for (WindowCapturer::WindowList::iterator new_list_it = windows.begin();
           new_list_it != windows.end(); ++new_list_it) {
        EXPECT_FALSE(it->id == new_list_it->id);
      }
      continue;
    }

    EXPECT_GT(frame_->size().width(), 0);
    EXPECT_GT(frame_->size().height(), 0);
  }
}

}  
