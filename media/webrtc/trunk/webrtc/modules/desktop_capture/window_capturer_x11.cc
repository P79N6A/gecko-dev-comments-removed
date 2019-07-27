









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include <assert.h>
#include <string.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>

#include <algorithm>

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_display.h"
#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"
#include "webrtc/modules/desktop_capture/x11/x_server_pixel_buffer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_util.h"

namespace webrtc {

namespace {

class WindowCapturerLinux : public WindowCapturer,
                            public SharedXDisplay::XEventHandler {
 public:
  WindowCapturerLinux(const DesktopCaptureOptions& options);
  virtual ~WindowCapturerLinux();

  
  virtual bool GetWindowList(WindowList* windows) OVERRIDE;
  virtual bool SelectWindow(WindowId id) OVERRIDE;
  virtual bool BringSelectedWindowToFront() OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

  
  virtual bool HandleXEvent(const XEvent& event) OVERRIDE;

 private:
  Display* display() { return x_display_->display(); }

  
  
  
  ::Window GetApplicationWindow(::Window window);

  
  bool IsDesktopElement(::Window window);

  
  bool GetWindowTitle(::Window window, std::string* title);

  Callback* callback_;

  scoped_refptr<SharedXDisplay> x_display_;

  Atom wm_state_atom_;
  Atom window_type_atom_;
  Atom normal_window_type_atom_;
  bool has_composite_extension_;

  ::Window selected_window_;
  XServerPixelBuffer x_server_pixel_buffer_;

  DISALLOW_COPY_AND_ASSIGN(WindowCapturerLinux);
};

WindowCapturerLinux::WindowCapturerLinux(const DesktopCaptureOptions& options)
    : callback_(NULL),
      x_display_(options.x_display()),
      has_composite_extension_(false),
      selected_window_(0) {
  
  wm_state_atom_ = XInternAtom(display(), "WM_STATE", True);
  window_type_atom_ = XInternAtom(display(), "_NET_WM_WINDOW_TYPE", True);
  normal_window_type_atom_ = XInternAtom(
      display(), "_NET_WM_WINDOW_TYPE_NORMAL", True);

  int event_base, error_base, major_version, minor_version;
  if (XCompositeQueryExtension(display(), &event_base, &error_base) &&
      XCompositeQueryVersion(display(), &major_version, &minor_version) &&
      
      (major_version > 0 || minor_version >= 2)) {
    has_composite_extension_ = true;
  } else {
    LOG(LS_INFO) << "Xcomposite extension not available or too old.";
  }

  x_display_->AddEventHandler(ConfigureNotify, this);
}

WindowCapturerLinux::~WindowCapturerLinux() {
  x_display_->RemoveEventHandler(ConfigureNotify, this);
}

bool WindowCapturerLinux::GetWindowList(WindowList* windows) {
  WindowList result;

  XErrorTrap error_trap(display());

  int num_screens = XScreenCount(display());
  for (int screen = 0; screen < num_screens; ++screen) {
    ::Window root_window = XRootWindow(display(), screen);
    ::Window parent;
    ::Window *children;
    unsigned int num_children;
    int status = XQueryTree(display(), root_window, &root_window, &parent,
                            &children, &num_children);
    if (status == 0) {
      LOG(LS_ERROR) << "Failed to query for child windows for screen "
                    << screen;
      continue;
    }

    for (unsigned int i = 0; i < num_children; ++i) {
      
      ::Window app_window =
          GetApplicationWindow(children[num_children - 1 - i]);
      if (app_window && !IsDesktopElement(app_window)) {
        Window w;
        w.id = app_window;
        if (GetWindowTitle(app_window, &w.title))
          result.push_back(w);
      }
    }

    if (children)
      XFree(children);
  }

  windows->swap(result);

  return true;
}

bool WindowCapturerLinux::SelectWindow(WindowId id) {
  if (!x_server_pixel_buffer_.Init(display(), id))
    return false;

  
  XSelectInput(display(), id, StructureNotifyMask);

  selected_window_ = id;

  
  
  
  

  
  
  XCompositeRedirectWindow(display(), id, CompositeRedirectAutomatic);

  return true;
}

bool WindowCapturerLinux::BringSelectedWindowToFront() {
  if (!selected_window_)
    return false;

  unsigned int num_children;
  ::Window* children;
  ::Window parent;
  ::Window root;
  
  int status = XQueryTree(
      display(), selected_window_, &root, &parent, &children, &num_children);
  if (status == 0) {
    LOG(LS_ERROR) << "Failed to query for the root window.";
    return false;
  }

  if (children)
    XFree(children);

  XRaiseWindow(display(), selected_window_);

  
  
  
  Atom atom = XInternAtom(display(), "_NET_ACTIVE_WINDOW", True);
  if (atom != None) {
    XEvent xev;
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = True;
    xev.xclient.window = selected_window_;
    xev.xclient.message_type = atom;

    
    
    xev.xclient.format = 32;

    memset(xev.xclient.data.l, 0, sizeof(xev.xclient.data.l));

    XSendEvent(display(),
               root,
               False,
               SubstructureRedirectMask | SubstructureNotifyMask,
               &xev);
  }
  XFlush(display());
  return true;
}

void WindowCapturerLinux::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void WindowCapturerLinux::Capture(const DesktopRegion& region) {
  x_display_->ProcessPendingXEvents();

  if (!x_server_pixel_buffer_.IsWindowValid()) {
    LOG(LS_INFO) << "The window is no longer valid.";
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  if (!has_composite_extension_) {
    
    
    
    LOG(LS_INFO) << "No Xcomposite extension detected.";
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  DesktopFrame* frame =
      new BasicDesktopFrame(x_server_pixel_buffer_.window_size());

  x_server_pixel_buffer_.Synchronize();
  x_server_pixel_buffer_.CaptureRect(DesktopRect::MakeSize(frame->size()),
                                     frame);

  frame->mutable_updated_region()->SetRect(
      DesktopRect::MakeSize(frame->size()));

  callback_->OnCaptureCompleted(frame);
}

bool WindowCapturerLinux::HandleXEvent(const XEvent& event) {
  if (event.type == ConfigureNotify) {
    XConfigureEvent xce = event.xconfigure;
    if (!DesktopSize(xce.width, xce.height).equals(
            x_server_pixel_buffer_.window_size())) {
      if (!x_server_pixel_buffer_.Init(display(), selected_window_)) {
        LOG(LS_ERROR) << "Failed to initialize pixel buffer after resizing.";
      }
      return true;
    }
  }
  return false;
}

::Window WindowCapturerLinux::GetApplicationWindow(::Window window) {
  
  XWindowProperty<uint32_t> window_state(display(), window, wm_state_atom_);

  
  int32_t state = window_state.is_valid() ?
      *window_state.data() : WithdrawnState;

  if (state == NormalState) {
    
    return window;
  } else if (state == IconicState) {
    
    return 0;
  }

  
  ::Window root, parent;
  ::Window *children;
  unsigned int num_children;
  if (!XQueryTree(display(), window, &root, &parent, &children,
                  &num_children)) {
    LOG(LS_ERROR) << "Failed to query for child windows although window"
                  << "does not have a valid WM_STATE.";
    return 0;
  }
  ::Window app_window = 0;
  for (unsigned int i = 0; i < num_children; ++i) {
    app_window = GetApplicationWindow(children[i]);
    if (app_window)
      break;
  }

  if (children)
    XFree(children);
  return app_window;
}

bool WindowCapturerLinux::IsDesktopElement(::Window window) {
  if (window == 0)
    return false;

  
  
  
  
  
  XWindowProperty<uint32_t> window_type(display(), window, window_type_atom_);
  if (window_type.is_valid() && window_type.size() > 0) {
    uint32_t* end = window_type.data() + window_type.size();
    bool is_normal = (end != std::find(
        window_type.data(), end, normal_window_type_atom_));
    return !is_normal;
  }

  
  XClassHint class_hint;
  Status status = XGetClassHint(display(), window, &class_hint);
  bool result = false;
  if (status == 0) {
    
    return result;
  }

  if (strcmp("gnome-panel", class_hint.res_name) == 0 ||
      strcmp("desktop_window", class_hint.res_name) == 0) {
    result = true;
  }
  XFree(class_hint.res_name);
  XFree(class_hint.res_class);
  return result;
}

bool WindowCapturerLinux::GetWindowTitle(::Window window, std::string* title) {
  int status;
  bool result = false;
  XTextProperty window_name;
  window_name.value = NULL;
  if (window) {
    status = XGetWMName(display(), window, &window_name);
    if (status && window_name.value && window_name.nitems) {
      int cnt;
      char **list = NULL;
      status = Xutf8TextPropertyToTextList(display(), &window_name, &list,
                                           &cnt);
      if (status >= Success && cnt && *list) {
        if (cnt > 1) {
          LOG(LS_INFO) << "Window has " << cnt
                       << " text properties, only using the first one.";
        }
        *title = *list;
        result = true;
      }
      if (list)
        XFreeStringList(list);
    }
    if (window_name.value)
      XFree(window_name.value);
  }
  return result;
}

}  


WindowCapturer* WindowCapturer::Create(const DesktopCaptureOptions& options) {
  if (!options.x_display())
    return NULL;
  return new WindowCapturerLinux(options);
}

}  
