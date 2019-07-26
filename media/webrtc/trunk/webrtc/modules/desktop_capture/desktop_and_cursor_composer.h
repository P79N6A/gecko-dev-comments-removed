









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_AND_CURSOR_COMPOSER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_AND_CURSOR_COMPOSER_H_

#include "webrtc/modules/desktop_capture/desktop_capturer.h"
#include "webrtc/modules/desktop_capture/mouse_cursor_monitor.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {



class DesktopAndCursorComposer : public DesktopCapturer,
                            public DesktopCapturer::Callback,
                            public MouseCursorMonitor::Callback {
 public:
  
  
  
  
  DesktopAndCursorComposer(DesktopCapturer* desktop_capturer,
                      MouseCursorMonitor* mouse_monitor);
  virtual ~DesktopAndCursorComposer();

  
  virtual void Start(DesktopCapturer::Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

 private:
  
  virtual SharedMemory* CreateSharedMemory(size_t size) OVERRIDE;
  virtual void OnCaptureCompleted(DesktopFrame* frame) OVERRIDE;

  
  virtual void OnMouseCursor(MouseCursor* cursor) OVERRIDE;
  virtual void OnMouseCursorPosition(MouseCursorMonitor::CursorState state,
                                     const DesktopVector& position) OVERRIDE;

  scoped_ptr<DesktopCapturer> desktop_capturer_;
  scoped_ptr<MouseCursorMonitor> mouse_monitor_;

  DesktopCapturer::Callback* callback_;

  scoped_ptr<MouseCursor> cursor_;
  MouseCursorMonitor::CursorState cursor_state_;
  DesktopVector cursor_position_;

  DISALLOW_COPY_AND_ASSIGN(DesktopAndCursorComposer);
};

}  

#endif  
