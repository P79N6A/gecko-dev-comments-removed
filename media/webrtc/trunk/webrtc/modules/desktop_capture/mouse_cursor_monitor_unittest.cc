









#include "webrtc/modules/desktop_capture/mouse_cursor_monitor.h"

#include "gtest/gtest.h"
#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/mouse_cursor.h"
#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class MouseCursorMonitorTest : public testing::Test,
                               public MouseCursorMonitor::Callback {
 public:
  MouseCursorMonitorTest()
      : position_received_(false) {
  }

  
  virtual void OnMouseCursor(MouseCursor* cursor_image) OVERRIDE {
    cursor_image_.reset(cursor_image);
  }

  virtual void OnMouseCursorPosition(MouseCursorMonitor::CursorState state,
                                     const DesktopVector& position) OVERRIDE {
    state_ = state;
    position_ = position;
    position_received_ = true;
  }

 protected:
  scoped_ptr<MouseCursor> cursor_image_;
  MouseCursorMonitor::CursorState state_;
  DesktopVector position_;
  bool position_received_;
};





#if !defined(WEBRTC_MAC)
#define MAYBE(x) x
#else
#define MAYBE(x) DISABLED_##x
#endif

TEST_F(MouseCursorMonitorTest, MAYBE(FromScreen)) {
  scoped_ptr<MouseCursorMonitor> capturer(MouseCursorMonitor::CreateForScreen(
      DesktopCaptureOptions::CreateDefault(), webrtc::kFullDesktopScreenId));
  assert(capturer.get());
  capturer->Init(this, MouseCursorMonitor::SHAPE_AND_POSITION);
  capturer->Capture();

  EXPECT_TRUE(cursor_image_.get());
  EXPECT_GE(cursor_image_->hotspot().x(), 0);
  EXPECT_LE(cursor_image_->hotspot().x(),
            cursor_image_->image()->size().width());
  EXPECT_GE(cursor_image_->hotspot().y(), 0);
  EXPECT_LE(cursor_image_->hotspot().y(),
            cursor_image_->image()->size().height());

  EXPECT_TRUE(position_received_);
  EXPECT_EQ(MouseCursorMonitor::INSIDE, state_);
}

TEST_F(MouseCursorMonitorTest, MAYBE(FromWindow)) {
  DesktopCaptureOptions options = DesktopCaptureOptions::CreateDefault();

  
  scoped_ptr<WindowCapturer> window_capturer(WindowCapturer::Create(options));

  
  if (!window_capturer.get())
    return;

  WindowCapturer::WindowList windows;
  EXPECT_TRUE(window_capturer->GetWindowList(&windows));

  
  for (size_t i = 0; i < windows.size(); ++i) {
    cursor_image_.reset();
    position_received_ = false;

    scoped_ptr<MouseCursorMonitor> capturer(
        MouseCursorMonitor::CreateForWindow(
            DesktopCaptureOptions::CreateDefault(), windows[i].id));
    assert(capturer.get());

    capturer->Init(this, MouseCursorMonitor::SHAPE_AND_POSITION);
    capturer->Capture();

    EXPECT_TRUE(cursor_image_.get());
    EXPECT_TRUE(position_received_);
  }
}


TEST_F(MouseCursorMonitorTest, MAYBE(ShapeOnly)) {
  scoped_ptr<MouseCursorMonitor> capturer(MouseCursorMonitor::CreateForScreen(
      DesktopCaptureOptions::CreateDefault(), webrtc::kFullDesktopScreenId));
  assert(capturer.get());
  capturer->Init(this, MouseCursorMonitor::SHAPE_ONLY);
  capturer->Capture();

  EXPECT_TRUE(cursor_image_.get());
  EXPECT_FALSE(position_received_);
}

}  
