



#ifndef BASE_WIN_SCOPED_HANDLE_H_
#define BASE_WIN_SCOPED_HANDLE_H_

#include <windows.h>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/move.h"

namespace base {
namespace win {


#if defined(COMPILER_MSVC)

extern "C" {
  void* _ReturnAddress();
}
#define BASE_WIN_GET_CALLER _ReturnAddress()
#elif defined(COMPILER_GCC)
#define BASE_WIN_GET_CALLER __builtin_extract_return_addr(\\
    __builtin_return_address(0))
#endif








template <class Traits, class Verifier>
class GenericScopedHandle {
  MOVE_ONLY_TYPE_FOR_CPP_03(GenericScopedHandle, RValue)

 public:
  typedef typename Traits::Handle Handle;

  
  
  class Receiver {
   public:
    explicit Receiver(GenericScopedHandle* owner)
        : handle_(Traits::NullHandle()),
          owner_(owner) {}
    ~Receiver() { owner_->Set(handle_); }

    operator Handle*() { return &handle_; }

   private:
    Handle handle_;
    GenericScopedHandle* owner_;
  };

  GenericScopedHandle() : handle_(Traits::NullHandle()) {}

  explicit GenericScopedHandle(Handle handle) : handle_(Traits::NullHandle()) {
    Set(handle);
  }

  
  GenericScopedHandle(RValue other) : handle_(Traits::NullHandle()) {
    Set(other.object->Take());
  }

  ~GenericScopedHandle() {
    Close();
  }

  bool IsValid() const {
    return Traits::IsHandleValid(handle_);
  }

  
  GenericScopedHandle& operator=(RValue other) {
    if (this != other.object) {
      Set(other.object->Take());
    }
    return *this;
  }

  void Set(Handle handle) {
    if (handle_ != handle) {
      Close();

      if (Traits::IsHandleValid(handle)) {
        handle_ = handle;
        Verifier::StartTracking(handle, this, BASE_WIN_GET_CALLER,
                                tracked_objects::GetProgramCounter());
      }
    }
  }

  Handle Get() const {
    return handle_;
  }

  operator Handle() const {
    return handle_;
  }

  
  
  
  
  
  Receiver Receive() {
    DCHECK(!Traits::IsHandleValid(handle_)) << "Handle must be NULL";
    return Receiver(this);
  }

  
  Handle Take() {
    Handle temp = handle_;
    handle_ = Traits::NullHandle();
    if (Traits::IsHandleValid(temp)) {
      Verifier::StopTracking(temp, this, BASE_WIN_GET_CALLER,
                             tracked_objects::GetProgramCounter());
    }
    return temp;
  }

  
  void Close() {
    if (Traits::IsHandleValid(handle_)) {
      Verifier::StopTracking(handle_, this, BASE_WIN_GET_CALLER,
                             tracked_objects::GetProgramCounter());

      if (!Traits::CloseHandle(handle_))
        CHECK(false);

      handle_ = Traits::NullHandle();
    }
  }

 private:
  Handle handle_;
};

#undef BASE_WIN_GET_CALLER


class HandleTraits {
 public:
  typedef HANDLE Handle;

  
  static bool CloseHandle(HANDLE handle) {
    return ::CloseHandle(handle) != FALSE;
  }

  
  static bool IsHandleValid(HANDLE handle) {
    return handle != NULL && handle != INVALID_HANDLE_VALUE;
  }

  
  static HANDLE NullHandle() {
    return NULL;
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HandleTraits);
};


class DummyVerifierTraits {
 public:
  typedef HANDLE Handle;

  static void StartTracking(HANDLE handle, const void* owner,
                            const void* pc1, const void* pc2) {}
  static void StopTracking(HANDLE handle, const void* owner,
                           const void* pc1, const void* pc2) {}

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DummyVerifierTraits);
};


class BASE_EXPORT VerifierTraits {
 public:
  typedef HANDLE Handle;

  static void StartTracking(HANDLE handle, const void* owner,
                            const void* pc1, const void* pc2);
  static void StopTracking(HANDLE handle, const void* owner,
                           const void* pc1, const void* pc2);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(VerifierTraits);
};

typedef GenericScopedHandle<HandleTraits, VerifierTraits> ScopedHandle;

}  
}  

#endif  
