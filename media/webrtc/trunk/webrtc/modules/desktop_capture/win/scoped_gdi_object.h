









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCOPED_GDI_HANDLE_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCOPED_GDI_HANDLE_H_

#include <windows.h>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace win {


template<class T, class Traits>
class ScopedGDIObject {
 public:
  ScopedGDIObject() : handle_(NULL) {}
  explicit ScopedGDIObject(T object) : handle_(object) {}

  ~ScopedGDIObject() {
    Traits::Close(handle_);
  }

  T Get() {
    return handle_;
  }

  void Set(T object) {
    if (handle_ && object != handle_)
      Traits::Close(handle_);
    handle_ = object;
  }

  ScopedGDIObject& operator=(T object) {
    Set(object);
    return *this;
  }

  T release() {
    T object = handle_;
    handle_ = NULL;
    return object;
  }

  operator T() { return handle_; }

 private:
  T handle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedGDIObject);
};


template <typename T>
class DeleteObjectTraits {
 public:
  
  static void Close(T handle) {
    if (handle)
      DeleteObject(handle);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DeleteObjectTraits);
};


class DestroyCursorTraits {
 public:
  
  static void Close(HCURSOR handle) {
    if (handle)
      DestroyCursor(handle);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DestroyCursorTraits);
};

typedef ScopedGDIObject<HBITMAP, DeleteObjectTraits<HBITMAP> > ScopedBitmap;
typedef ScopedGDIObject<HCURSOR, DestroyCursorTraits> ScopedCursor;

}  
}  

#endif  
