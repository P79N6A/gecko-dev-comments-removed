








#include "webrtc/modules/desktop_capture/app_capturer.h"

#include "gtest/gtest.h"
#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class AppCapturerTest : public testing::Test,
                        public DesktopCapturer::Callback {
public:
  void SetUp() OVERRIDE {
    capturer_.reset(
      AppCapturer::Create(DesktopCaptureOptions::CreateDefault())
    );
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
  scoped_ptr<AppCapturer> capturer_;
  scoped_ptr<DesktopFrame> frame_;
};


TEST_F(AppCapturerTest, Enumerate) {
  AppCapturer::AppList apps;
  EXPECT_TRUE(capturer_->GetAppList(&apps));

  
  for (AppCapturer::AppList::iterator it = apps.begin();
       it != windows.end(); ++it) {
    EXPECT_FALSE(it->title.empty());
  }
}


TEST_F(AppCapturerTest, Capture) {
  AppCapturer::AppList apps;
  capturer_->Start(this);
  EXPECT_TRUE(capturer_->GetAppList(&apps));

  
  for (AppCapturer::AppList::iterator it = apps.begin();
       it != apps.end(); ++it) {
    frame_.reset();
    if (capturer_->SelectApp(it->id)) {
      capturer_->Capture(DesktopRegion());
    }

    
    if (!frame_.get()) {
      AppCapturer::AppList new_list;
      EXPECT_TRUE(capturer_->GetAppList(&new_list));
      for (AppCapturer::AppList::iterator new_list_it = apps.begin();
           new_list_it != apps.end(); ++new_list_it) {
        EXPECT_FALSE(it->id == new_list_it->id);
      }
      continue;
    }

    EXPECT_GT(frame_->size().width(), 0);
    EXPECT_GT(frame_->size().height(), 0);
  }
}

}  
