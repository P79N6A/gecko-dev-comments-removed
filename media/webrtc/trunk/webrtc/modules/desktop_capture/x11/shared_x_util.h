









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_X11_SHARED_X_UTIL_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_X11_SHARED_X_UTIL_H_

#include <map>
#include <vector>

#include <assert.h>
#include <X11/Xlib.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>
#include <string>

#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"

namespace webrtc {





template <class PropertyType>
class XWindowProperty {
public:
  XWindowProperty(Display* display, Window window, Atom property) :
      is_valid_(false),
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
    if (data_) {
      XFree(data_);
    }
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

class WindowUtilX11 {
public:
  WindowUtilX11(scoped_refptr<SharedXDisplay> x_display);
  ~WindowUtilX11();
  
  
  
  ::Window GetApplicationWindow(::Window window);
  
  bool IsDesktopElement(::Window window);
  
  bool GetWindowTitle(::Window window, std::string* title);
  bool BringWindowToFront(::Window window);
  int GetWindowProcessID(::Window window);
  int32_t GetWindowStatus(::Window window);
  bool GetWindowRect(::Window window, XRectangle &rcWindow,bool bWithFrame);
  bool GetWindowFrameExtents(::Window window, int32_t &left, int32_t &top, int32_t &right, int32_t &bottom);

protected:
  Display* display() { return x_display_->display(); }

  scoped_refptr<SharedXDisplay> x_display_;
  Atom wm_state_atom_;
  Atom window_type_atom_;
  Atom normal_window_type_atom_;
  Atom process_atom_;
  Atom frame_extends_atom_;
};

}  

#endif  
