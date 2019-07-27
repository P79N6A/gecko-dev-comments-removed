



#ifndef SANDBOX_WIN_SRC_SANDBOX_POLICY_H_
#define SANDBOX_WIN_SRC_SANDBOX_POLICY_H_

#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/security_level.h"

namespace sandbox {

class TargetPolicy {
 public:
  
  
  
  
  enum SubSystem {
    SUBSYS_FILES,             
    SUBSYS_NAMED_PIPES,       
    SUBSYS_PROCESS,           
    SUBSYS_REGISTRY,          
    SUBSYS_SYNC,              
    SUBSYS_HANDLES,           
    SUBSYS_WIN32K_LOCKDOWN    
  };

  
  enum Semantics {
    FILES_ALLOW_ANY,       
                           
    FILES_ALLOW_READONLY,  
    FILES_ALLOW_QUERY,     
    FILES_ALLOW_DIR_ANY,   
                           
    HANDLES_DUP_ANY,       
                           
    HANDLES_DUP_BROKER,    
    NAMEDPIPES_ALLOW_ANY,  
    PROCESS_MIN_EXEC,      
                           
                           
                           
    PROCESS_ALL_EXEC,      
                           
                           
                           
    EVENTS_ALLOW_ANY,      
    EVENTS_ALLOW_READONLY, 
    REG_ALLOW_READONLY,    
    REG_ALLOW_ANY,         
    FAKE_USER_GDI_INIT     
                           
                           
  };

  
  
  virtual void AddRef() = 0;

  
  
  
  
  virtual void Release() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual ResultCode SetTokenLevel(TokenLevel initial, TokenLevel lockdown) = 0;

  
  virtual TokenLevel GetInitialTokenLevel() const = 0;

  
  virtual TokenLevel GetLockdownTokenLevel() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual ResultCode SetJobLevel(JobLevel job_level, uint32 ui_exceptions) = 0;

  
  
  
  virtual ResultCode SetJobMemoryLimit(size_t memory_limit) = 0;

  
  
  
  virtual ResultCode SetAlternateDesktop(bool alternate_winstation) = 0;

  
  
  
  virtual base::string16 GetAlternateDesktop() const = 0;

  
  virtual ResultCode CreateAlternateDesktop(bool alternate_winstation) = 0;

  
  virtual void DestroyAlternateDesktop() = 0;

  
  
  
  
  virtual ResultCode SetIntegrityLevel(IntegrityLevel level) = 0;

  
  virtual IntegrityLevel GetIntegrityLevel() const = 0;

  
  
  
  
  
  virtual ResultCode SetDelayedIntegrityLevel(IntegrityLevel level) = 0;

  
  
  
  
  
  
  
  
  
  virtual ResultCode SetAppContainer(const wchar_t* sid) = 0;

  
  virtual ResultCode SetCapability(const wchar_t* sid) = 0;

  
  
  
  
  virtual ResultCode SetProcessMitigations(MitigationFlags flags) = 0;

  
  virtual MitigationFlags GetProcessMitigations() = 0;

  
  
  virtual ResultCode SetDelayedProcessMitigations(MitigationFlags flags) = 0;

  
  virtual MitigationFlags GetDelayedProcessMitigations() const = 0;

  
  
  
  
  
  virtual void SetStrictInterceptions() = 0;

  
  
  
  
  
  virtual ResultCode SetStdoutHandle(HANDLE handle) = 0;
  virtual ResultCode SetStderrHandle(HANDLE handle) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual ResultCode AddRule(SubSystem subsystem, Semantics semantics,
                             const wchar_t* pattern) = 0;

  
  
  
  virtual ResultCode AddDllToUnload(const wchar_t* dll_name) = 0;

  
  
  
  virtual ResultCode AddKernelObjectToClose(const wchar_t* handle_type,
                                            const wchar_t* handle_name) = 0;
};

}  


#endif  
