







#ifndef BASE_DEBUG_UTIL_H_
#define BASE_DEBUG_UTIL_H_

#include <iostream>
#include <vector>

#include "base/basictypes.h"




class StackTrace {
 public:
  
  StackTrace();
  
  
  const void *const *Addresses(size_t* count);
  
  void PrintBacktrace();

  
  void OutputToStream(std::ostream* os);

 private:
  std::vector<void*> trace_;

  DISALLOW_EVIL_CONSTRUCTORS(StackTrace);
};

class DebugUtil {
 public:
  
  
  static bool SpawnDebuggerOnProcess(unsigned process_id);

  
  
  static bool WaitForDebugger(int wait_seconds, bool silent);

  
  
  
  
  
  static bool BeingDebugged();

  
  static void BreakDebugger();

#if defined(OS_MACOSX)
  
  
  
  
  
  
  static void DisableOSCrashDumps();
#endif  
};

namespace mozilla {

class EnvironmentLog
{
public:
  EnvironmentLog(const char* varname);
  ~EnvironmentLog();

  void print(const char* format, ...);

private:
  FILE* fd_;

  DISALLOW_EVIL_CONSTRUCTORS(EnvironmentLog);
};

} 

#endif  
