



#ifndef SANDBOX_LINUX_SECCOMP_BPF_DIE_H__
#define SANDBOX_LINUX_SECCOMP_BPF_DIE_H__

#include "base/macros.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {



#define SANDBOX_DIE(m) sandbox::Die::SandboxDie(m, __FILE__, __LINE__)



#define RAW_SANDBOX_DIE(m) sandbox::Die::RawSandboxDie(m)


#define SANDBOX_INFO(m) sandbox::Die::SandboxInfo(m, __FILE__, __LINE__)

class SANDBOX_EXPORT Die {
 public:
  
  
  
  
  
  static void ExitGroup() __attribute__((noreturn));

  
  
  static void SandboxDie(const char* msg, const char* file, int line)
      __attribute__((noreturn));

  static void RawSandboxDie(const char* msg) __attribute__((noreturn));

  
  
  static void SandboxInfo(const char* msg, const char* file, int line);

  
  
  static void LogToStderr(const char* msg, const char* file, int line);

  
  
  
  
  
  static void EnableSimpleExit() { simple_exit_ = true; }

  
  
  static void SuppressInfoMessages(bool flag) { suppress_info_ = flag; }

 private:
  static bool simple_exit_;
  static bool suppress_info_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(Die);
};

}  

#endif  
