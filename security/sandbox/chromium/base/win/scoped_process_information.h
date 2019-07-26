



#ifndef BASE_WIN_SCOPED_PROCESS_INFORMATION_H_
#define BASE_WIN_SCOPED_PROCESS_INFORMATION_H_

#include <windows.h>

#include "base/basictypes.h"
#include "base/base_export.h"
#include "base/win/scoped_handle.h"

namespace base {
namespace win {



class BASE_EXPORT ScopedProcessInformation {
 public:
  
  
  class Receiver {
   public:
    explicit Receiver(ScopedProcessInformation* owner)
        : info_(),
          owner_(owner) {}
    ~Receiver() { owner_->Set(info_); }

    operator PROCESS_INFORMATION*() { return &info_; }

   private:
    PROCESS_INFORMATION info_;
    ScopedProcessInformation* owner_;
  };

  ScopedProcessInformation();
  ~ScopedProcessInformation();

  
  
  
  
  
  
  Receiver Receive();

  
  bool IsValid() const;

  
  void Close();

  
  void Set(const PROCESS_INFORMATION& process_info);

  
  
  
  bool DuplicateFrom(const ScopedProcessInformation& other);

  
  
  PROCESS_INFORMATION Take();

  
  
  HANDLE TakeProcessHandle();

  
  
  HANDLE TakeThreadHandle();

  
  HANDLE process_handle() const {
    return process_handle_.Get();
  }

  
  HANDLE thread_handle() const {
    return thread_handle_.Get();
  }

  
  DWORD process_id() const {
    return process_id_;
  }

  
  DWORD thread_id() const {
    return thread_id_;
  }

 private:
  ScopedHandle process_handle_;
  ScopedHandle thread_handle_;
  DWORD process_id_;
  DWORD thread_id_;

  DISALLOW_COPY_AND_ASSIGN(ScopedProcessInformation);
};

}  
}  

#endif  
