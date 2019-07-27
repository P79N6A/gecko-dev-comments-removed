



#ifndef BASE_PROCESS_PROCESS_HANDLE_H_
#define BASE_PROCESS_PROCESS_HANDLE_H_

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "build/build_config.h"

#include <sys/types.h>
#if defined(OS_WIN)
#include <windows.h>
#endif

namespace base {




#if defined(OS_WIN)
typedef HANDLE ProcessHandle;
typedef DWORD ProcessId;
typedef HANDLE UserTokenHandle;
const ProcessHandle kNullProcessHandle = NULL;
const ProcessId kNullProcessId = 0;
#elif defined(OS_POSIX)

typedef pid_t ProcessHandle;
typedef pid_t ProcessId;
const ProcessHandle kNullProcessHandle = 0;
const ProcessId kNullProcessId = 0;
#endif  


BASE_EXPORT ProcessId GetCurrentProcId();


BASE_EXPORT ProcessHandle GetCurrentProcessHandle();



BASE_EXPORT bool OpenProcessHandle(ProcessId pid, ProcessHandle* handle);







BASE_EXPORT bool OpenPrivilegedProcessHandle(ProcessId pid,
                                             ProcessHandle* handle);



BASE_EXPORT bool OpenProcessHandleWithAccess(ProcessId pid,
                                             uint32 access_flags,
                                             ProcessHandle* handle);


BASE_EXPORT void CloseProcessHandle(ProcessHandle process);




BASE_EXPORT ProcessId GetProcId(ProcessHandle process);

#if defined(OS_WIN)
enum IntegrityLevel {
  INTEGRITY_UNKNOWN,
  LOW_INTEGRITY,
  MEDIUM_INTEGRITY,
  HIGH_INTEGRITY,
};



BASE_EXPORT bool GetProcessIntegrityLevel(ProcessHandle process,
                                          IntegrityLevel* level);
#endif

#if defined(OS_POSIX)

BASE_EXPORT FilePath GetProcessExecutablePath(ProcessHandle process);


BASE_EXPORT ProcessId GetParentProcessId(ProcessHandle process);
#endif

}  

#endif  
