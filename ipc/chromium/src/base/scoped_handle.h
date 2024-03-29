



#ifndef BASE_SCOPED_HANDLE_H_
#define BASE_SCOPED_HANDLE_H_

#include <stdio.h>

#include "base/basictypes.h"

#if defined(OS_WIN)
#include "base/scoped_handle_win.h"
#endif

class ScopedStdioHandle {
 public:
  ScopedStdioHandle()
      : handle_(NULL) { }

  explicit ScopedStdioHandle(FILE* handle)
      : handle_(handle) { }

  ~ScopedStdioHandle() {
    Close();
  }

  void Close() {
    if (handle_) {
      fclose(handle_);
      handle_ = NULL;
    }
  }

  FILE* get() const { return handle_; }

  FILE* Take() {
    FILE* temp = handle_;
    handle_ = NULL;
    return temp;
  }

  void Set(FILE* newhandle) {
    Close();
    handle_ = newhandle;
  }

 private:
  FILE* handle_;

  DISALLOW_EVIL_CONSTRUCTORS(ScopedStdioHandle);
};

#endif 
