



#ifndef BASE_SCOPED_HANDLE_WIN_H_
#define BASE_SCOPED_HANDLE_WIN_H_

#include <windows.h>

#include "base/basictypes.h"
#include "base/logging.h"

















class ScopedHandle {
 public:
  ScopedHandle() : handle_(NULL) {
  }

  explicit ScopedHandle(HANDLE h) : handle_(NULL) {
    Set(h);
  }

  ~ScopedHandle() {
    Close();
  }

  
  
  bool IsValid() const {
    return handle_ != NULL;
  }

  void Set(HANDLE new_handle) {
    Close();

    
    if (new_handle != INVALID_HANDLE_VALUE)
      handle_ = new_handle;
  }

  HANDLE Get() {
    return handle_;
  }

  operator HANDLE() { return handle_; }

  HANDLE Take() {
    
    HANDLE h = handle_;
    handle_ = NULL;
    return h;
  }

  void Close() {
    if (handle_) {
      if (!::CloseHandle(handle_)) {
        NOTREACHED();
      }
      handle_ = NULL;
    }
  }

 private:
  HANDLE handle_;
  DISALLOW_EVIL_CONSTRUCTORS(ScopedHandle);
};


class ScopedFindFileHandle {
 public:
  explicit ScopedFindFileHandle(HANDLE handle) : handle_(handle) {
    
    if (handle_ == INVALID_HANDLE_VALUE)
      handle_ = NULL;
  }

  ~ScopedFindFileHandle() {
    if (handle_)
      FindClose(handle_);
  }

  
  
  bool IsValid() const { return handle_ != NULL; }

  operator HANDLE() { return handle_; }

 private:
  HANDLE handle_;

  DISALLOW_EVIL_CONSTRUCTORS(ScopedFindFileHandle);
};



class ScopedHDC {
 public:
  ScopedHDC() : hdc_(NULL) { }
  explicit ScopedHDC(HDC h) : hdc_(h) { }

  ~ScopedHDC() {
    Close();
  }

  HDC Get() {
    return hdc_;
  }

  void Set(HDC h) {
    Close();
    hdc_ = h;
  }

  operator HDC() { return hdc_; }

 private:
  void Close() {
#ifdef NOGDI
    assert(false);
#else
    if (hdc_)
      DeleteDC(hdc_);
#endif  
  }

  HDC hdc_;
  DISALLOW_EVIL_CONSTRUCTORS(ScopedHDC);
};


template<class T>
class ScopedGDIObject {
 public:
  ScopedGDIObject() : object_(NULL) {}
  explicit ScopedGDIObject(T object) : object_(object) {}

  ~ScopedGDIObject() {
    Close();
  }

  T Get() {
    return object_;
  }

  void Set(T object) {
    if (object_ && object != object_)
      Close();
    object_ = object;
  }

  ScopedGDIObject& operator=(T object) {
    Set(object);
    return *this;
  }

  operator T() { return object_; }

 private:
  void Close() {
    if (object_)
      DeleteObject(object_);
  }

  T object_;
  DISALLOW_COPY_AND_ASSIGN(ScopedGDIObject);
};


typedef ScopedGDIObject<HBITMAP> ScopedBitmap;
typedef ScopedGDIObject<HRGN> ScopedHRGN;
typedef ScopedGDIObject<HFONT> ScopedHFONT;



template<class T>
class ScopedHGlobal {
 public:
  explicit ScopedHGlobal(HGLOBAL glob) : glob_(glob) {
    data_ = static_cast<T*>(GlobalLock(glob_));
  }
  ~ScopedHGlobal() {
    GlobalUnlock(glob_);
  }

  T* get() { return data_; }

  size_t Size() const { return GlobalSize(glob_); }

  T* operator->() const  {
    assert(data_ != 0);
    return data_;
  }

 private:
  HGLOBAL glob_;

  T* data_;

  DISALLOW_EVIL_CONSTRUCTORS(ScopedHGlobal);
};

#endif 
