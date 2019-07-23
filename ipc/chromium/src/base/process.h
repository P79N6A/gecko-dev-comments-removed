



#ifndef BASE_PROCESS_H_
#define BASE_PROCESS_H_

#include "base/basictypes.h"

#include <sys/types.h>
#ifdef OS_WIN
#include <windows.h>
#endif

namespace base {




#if defined(OS_WIN)
typedef HANDLE ProcessHandle;
typedef DWORD ProcessId;
#elif defined(OS_POSIX)

typedef pid_t ProcessHandle;
typedef pid_t ProcessId;
#endif

class Process {
 public:
  Process() : process_(0), last_working_set_size_(0) {}
  explicit Process(ProcessHandle handle) :
    process_(handle), last_working_set_size_(0) {}

  
  static Process Current();

  
  
  ProcessHandle handle() const { return process_; }
  void set_handle(ProcessHandle handle) { process_ = handle; }

  
  ProcessId pid() const;

  
  bool is_current() const;

  
  void Close();

  
  
  
  void Terminate(int result_code);

  
  
  bool IsProcessBackgrounded() const;

  
  
  
  
  
  bool SetProcessBackgrounded(bool value);

  
  
  
  
  
  
  
  
  bool ReduceWorkingSet();

  
  
  bool UnReduceWorkingSet();

  
  
  bool EmptyWorkingSet();

 private:
  ProcessHandle process_;
  size_t last_working_set_size_;
};

}  

#endif  
