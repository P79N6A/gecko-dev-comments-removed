









#include "webrtc/modules/desktop_capture/window_capturer.h"

#include <string.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <cassert>

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_display.h"
#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"
#include "webrtc/modules/desktop_capture/x11/x_server_pixel_buffer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"

namespace webrtc {

namespace {


template <class PropertyType>
class XWindowProperty {
 public:
  XWindowProperty(Display* display, Window window, Atom property)
      : is_valid_(false),
        size_(0),
        data_(NULL) {
    const int kBitsPerByte = 8;
    Atom actual_type;
    int actual_format;
    unsigned long bytes_after;  
    int status = XGetWindowProperty(display, window, property, 0L, ~0L, False,
                                    AnyPropertyType, &actual_type,
                                    &actual_format, &size_,
                                    &bytes_after, &data_);
    if (status != Success) {
      data_ = NULL;
      return;
    }
    if (sizeof(PropertyType) * kBitsPerByte != actual_format) {
      size_ = 0;
      return;
    }

    is_valid_ = true;
  }

  ~XWindowProperty() {
    if (data_)
      XFree(data_);
  }

  
  bool is_valid() const { return is_valid_; }

  
  size_t size() const { return size_; }
  const PropertyType* data() const {
    return reinterpret_cast<PropertyType*>(data_);
  }
  PropertyType* data() {
    return reinterpret_cast<PropertyType*>(data_);
  }

 private:
  bool is_valid_;
  unsigned long size_;  
  unsigned char* data_;

  DISALLOW_COPY_AND_ASSIGN(XWindowProperty);
};

class WindowCapturerLinux : public WindowCapturer {
 public:
  WindowCapturerLinux(const DesktopCaptureOptions& options);
  virtual ~WindowCapturerLinux();

  
  virtual bool GetWindowList(WindowList* windows) OVERRIDE;
  virtual bool SelectWindow(WindowId id) OVERRIDE;

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;

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
}

WindowCapturerLinux::~WindowCapturerLinux() {}

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

  selected_window_ = id;

  
  
  
  

  
  
  XCompositeRedirectWindow(display(), id, CompositeRedirectAutomatic);

  return true;
}

void WindowCapturerLinux::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void WindowCapturerLinux::Capture(const DesktopRegion& region) {
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

  callback_->OnCaptureCompleted(frame);
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
