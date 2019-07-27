









#include "webrtc/modules/desktop_capture/mouse_cursor_monitor.h"

#include <X11/extensions/Xfixes.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/mouse_cursor.h"
#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace {







Window GetTopLevelWindow(Display* display, Window window) {
  webrtc::XErrorTrap error_trap(display);
  while (true) {
    
    ::Window root, parent;
    ::Window *children;
    unsigned int num_children;
    if (!XQueryTree(display, window, &root, &parent, &children,
                    &num_children)) {
      LOG(LS_ERROR) << "Failed to query for child windows although window"
                    << "does not have a valid WM_STATE.";
      return None;
    }
    if (children)
      XFree(children);

    if (parent == root)
      break;

    window = parent;
  }

  return window;
}

}  

namespace webrtc {

class MouseCursorMonitorX11 : public MouseCursorMonitor,
                              public SharedXDisplay::XEventHandler {
 public:
  MouseCursorMonitorX11(const DesktopCaptureOptions& options, Window window);
  virtual ~MouseCursorMonitorX11();

  virtual void Init(Callback* callback, Mode mode) OVERRIDE;
  virtual void Capture() OVERRIDE;

 private:
  
  virtual bool HandleXEvent(const XEvent& event) OVERRIDE;

  Display* display() { return x_display_->display(); }

  
  void CaptureCursor();

  scoped_refptr<SharedXDisplay> x_display_;
  Callback* callback_;
  Mode mode_;
  Window window_;

  bool have_xfixes_;
  int xfixes_event_base_;
  int xfixes_error_base_;

  scoped_ptr<MouseCursor> cursor_shape_;
};

MouseCursorMonitorX11::MouseCursorMonitorX11(
    const DesktopCaptureOptions& options,
    Window window)
    : x_display_(options.x_display()),
      callback_(NULL),
      mode_(SHAPE_AND_POSITION),
      window_(window),
      have_xfixes_(false),
      xfixes_event_base_(-1),
      xfixes_error_base_(-1) {}

MouseCursorMonitorX11::~MouseCursorMonitorX11() {
  if (have_xfixes_) {
    x_display_->RemoveEventHandler(xfixes_event_base_ + XFixesCursorNotify,
                                   this);
  }
}

void MouseCursorMonitorX11::Init(Callback* callback, Mode mode) {
  
  assert(!callback_);
  assert(callback);

  callback_ = callback;
  mode_ = mode;

  have_xfixes_ =
      XFixesQueryExtension(display(), &xfixes_event_base_, &xfixes_error_base_);

  if (have_xfixes_) {
    
    XErrorTrap error_trap(display());
    XFixesSelectCursorInput(display(), window_, XFixesDisplayCursorNotifyMask);
    x_display_->AddEventHandler(xfixes_event_base_ + XFixesCursorNotify, this);

    CaptureCursor();
  } else {
    LOG(LS_INFO) << "X server does not support XFixes.";
  }
}

void MouseCursorMonitorX11::Capture() {
  assert(callback_);

  
  x_display_->ProcessPendingXEvents();

  
  if (cursor_shape_.get())
    callback_->OnMouseCursor(cursor_shape_.release());

  
  if (mode_ == SHAPE_AND_POSITION) {
    int root_x;
    int root_y;
    int win_x;
    int win_y;
    Window root_window;
    Window child_window;
    unsigned int mask;

    XErrorTrap error_trap(display());
    Bool result = XQueryPointer(display(), window_, &root_window, &child_window,
                                &root_x, &root_y, &win_x, &win_y, &mask);
    CursorState state;
    if (!result || error_trap.GetLastErrorAndDisable() != 0) {
      state = OUTSIDE;
    } else {
      
      
      
      state =
          (window_ == root_window || child_window != None) ? INSIDE : OUTSIDE;
    }

    callback_->OnMouseCursorPosition(state,
                                     webrtc::DesktopVector(win_x, win_y));
  }
}

bool MouseCursorMonitorX11::HandleXEvent(const XEvent& event) {
  if (have_xfixes_ && event.type == xfixes_event_base_ + XFixesCursorNotify) {
    const XFixesCursorNotifyEvent* cursor_event =
        reinterpret_cast<const XFixesCursorNotifyEvent*>(&event);
    if (cursor_event->subtype == XFixesDisplayCursorNotify) {
      CaptureCursor();
    }
    
    
  }
  return false;
}

void MouseCursorMonitorX11::CaptureCursor() {
  assert(have_xfixes_);

  XFixesCursorImage* img = XFixesGetCursorImage(display());
  if (!img)
     return;

  scoped_ptr<DesktopFrame> image(
      new BasicDesktopFrame(DesktopSize(img->width, img->height)));

  
  unsigned long* src = img->pixels;
  uint32_t* dst = reinterpret_cast<uint32_t*>(image->data());
  uint32_t* dst_end = dst + (img->width * img->height);
  while (dst < dst_end) {
    *dst++ = static_cast<uint32_t>(*src++);
  }

  DesktopVector hotspot(std::min(img->width, img->xhot),
                        std::min(img->height, img->yhot));

  XFree(img);

  cursor_shape_.reset(new MouseCursor(image.release(), hotspot));
}


MouseCursorMonitor* MouseCursorMonitor::CreateForWindow(
    const DesktopCaptureOptions& options, WindowId window) {
  if (!options.x_display())
    return NULL;
  window = GetTopLevelWindow(options.x_display()->display(), window);
  if (window == None)
    return NULL;
  return new MouseCursorMonitorX11(options, window);
}

MouseCursorMonitor* MouseCursorMonitor::CreateForScreen(
    const DesktopCaptureOptions& options,
    ScreenId screen) {
  if (!options.x_display())
    return NULL;
  return new MouseCursorMonitorX11(
      options, DefaultRootWindow(options.x_display()->display()));
}

}  
