



#ifndef SANDBOX_SRC_WOW64_H__
#define SANDBOX_SRC_WOW64_H__

#include <windows.h>

#include "base/basictypes.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {

class TargetProcess;



class Wow64 {
 public:
  Wow64(TargetProcess* child, HMODULE ntdll)
      : child_(child), ntdll_(ntdll), dll_load_(NULL), continue_load_(NULL) {}
  ~Wow64();

  
  
  
  bool WaitForNtdll();

 private:
  
  
  bool RunWowHelper(void* buffer);

  
  bool DllMapped();

  
  bool NtdllPresent();

  TargetProcess* child_;  
  HMODULE ntdll_;         
  HANDLE dll_load_;       
  HANDLE continue_load_;  
  DISALLOW_IMPLICIT_CONSTRUCTORS(Wow64);
};

}  

#endif  
