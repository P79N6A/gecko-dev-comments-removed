




#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include "base/process_util.h"

#include <windows.h>
#include <winternl.h>
#include <psapi.h>

#include "base/histogram.h"
#include "base/logging.h"
#include "base/win_util.h"

#include <algorithm>

namespace {


const int PAGESIZE_KB = 4;


typedef BOOL (WINAPI* HeapSetFn)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);

typedef BOOL (WINAPI * InitializeProcThreadAttributeListFn)(
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  DWORD dwAttributeCount,
  DWORD dwFlags,
  PSIZE_T lpSize
);

typedef BOOL (WINAPI * DeleteProcThreadAttributeListFn)(
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList
);

typedef BOOL (WINAPI * UpdateProcThreadAttributeFn)(
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  DWORD dwFlags,
  DWORD_PTR Attribute,
  PVOID lpValue,
  SIZE_T cbSize,
  PVOID lpPreviousValue,
  PSIZE_T lpReturnSize
);

static InitializeProcThreadAttributeListFn InitializeProcThreadAttributeListPtr;
static DeleteProcThreadAttributeListFn DeleteProcThreadAttributeListPtr;
static UpdateProcThreadAttributeFn UpdateProcThreadAttributePtr;

static mozilla::EnvironmentLog gProcessLog("MOZ_PROCESS_LOG");

}  

namespace base {

ProcessId GetCurrentProcId() {
  return ::GetCurrentProcessId();
}

ProcessHandle GetCurrentProcessHandle() {
  return ::GetCurrentProcess();
}

bool OpenProcessHandle(ProcessId pid, ProcessHandle* handle) {
  
  ProcessHandle result = OpenProcess(PROCESS_DUP_HANDLE |
                                         PROCESS_TERMINATE |
                                         PROCESS_QUERY_INFORMATION |
                                         SYNCHRONIZE,
                                     FALSE, pid);

  if (result == INVALID_HANDLE_VALUE)
    return false;

  *handle = result;
  return true;
}

bool OpenPrivilegedProcessHandle(ProcessId pid, ProcessHandle* handle) {
  ProcessHandle result = OpenProcess(PROCESS_DUP_HANDLE |
                                         PROCESS_TERMINATE |
                                         PROCESS_QUERY_INFORMATION |
                                         PROCESS_VM_READ |
                                         SYNCHRONIZE,
                                     FALSE, pid);

  if (result == INVALID_HANDLE_VALUE)
    return false;

  *handle = result;
  return true;
}

void CloseProcessHandle(ProcessHandle process) {
  
  
  
  BOOL ok = CloseHandle(process);
  DCHECK(ok);
}


bool GetProcIdViaGetProcessId(ProcessHandle process, DWORD* id) {
  
  typedef DWORD (WINAPI *GetProcessIdFunction)(HANDLE);
  static GetProcessIdFunction GetProcessIdPtr = NULL;
  static bool initialize_get_process_id = true;
  if (initialize_get_process_id) {
    initialize_get_process_id = false;
    HMODULE kernel32_handle = GetModuleHandle(L"kernel32.dll");
    if (!kernel32_handle) {
      NOTREACHED();
      return false;
    }
    GetProcessIdPtr = reinterpret_cast<GetProcessIdFunction>(GetProcAddress(
        kernel32_handle, "GetProcessId"));
  }
  if (!GetProcessIdPtr)
    return false;
  
  *id = (*GetProcessIdPtr)(process);
  return true;
}


bool GetProcIdViaNtQueryInformationProcess(ProcessHandle process, DWORD* id) {
  
  typedef NTSTATUS (WINAPI *NtQueryInformationProcessFunction)(
      HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
  static NtQueryInformationProcessFunction NtQueryInformationProcessPtr = NULL;
  static bool initialize_query_information_process = true;
  if (initialize_query_information_process) {
    initialize_query_information_process = false;
    
    
    HMODULE ntdll_handle = GetModuleHandle(L"ntdll.dll");
    if (!ntdll_handle) {
      NOTREACHED();
      return false;
    }
    NtQueryInformationProcessPtr =
        reinterpret_cast<NtQueryInformationProcessFunction>(GetProcAddress(
            ntdll_handle, "NtQueryInformationProcess"));
  }
  if (!NtQueryInformationProcessPtr)
    return false;
  
  PROCESS_BASIC_INFORMATION info;
  ULONG bytes_returned;
  NTSTATUS status = (*NtQueryInformationProcessPtr)(process,
                                                    ProcessBasicInformation,
                                                    &info, sizeof info,
                                                    &bytes_returned);
  if (!SUCCEEDED(status) || (bytes_returned != (sizeof info)))
    return false;

  *id = static_cast<DWORD>(info.UniqueProcessId);
  return true;
}

ProcessId GetProcId(ProcessHandle process) {
  
  HANDLE current_process = GetCurrentProcess();
  HANDLE process_with_query_rights;
  if (DuplicateHandle(current_process, process, current_process,
                      &process_with_query_rights, PROCESS_QUERY_INFORMATION,
                      false, 0)) {
    
    
    DWORD id;
    bool success =
        GetProcIdViaGetProcessId(process_with_query_rights, &id) ||
        GetProcIdViaNtQueryInformationProcess(process_with_query_rights, &id);
    CloseHandle(process_with_query_rights);
    if (success)
      return id;
  }

  
  NOTREACHED();
  return 0;
}


bool IsInheritableHandle(HANDLE handle) {
  if (!handle)
    return false;
  if (handle == INVALID_HANDLE_VALUE)
    return false;
  
  
  
  DWORD handle_type = GetFileType(handle);
  return handle_type == FILE_TYPE_DISK || handle_type == FILE_TYPE_PIPE;
}

void LoadThreadAttributeFunctions() {
  HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");
  InitializeProcThreadAttributeListPtr =
    reinterpret_cast<InitializeProcThreadAttributeListFn>
    (GetProcAddress(kernel32, "InitializeProcThreadAttributeList"));
  DeleteProcThreadAttributeListPtr =
    reinterpret_cast<DeleteProcThreadAttributeListFn>
    (GetProcAddress(kernel32, "DeleteProcThreadAttributeList"));
  UpdateProcThreadAttributePtr =
    reinterpret_cast<UpdateProcThreadAttributeFn>
    (GetProcAddress(kernel32, "UpdateProcThreadAttribute"));
}









LPPROC_THREAD_ATTRIBUTE_LIST CreateThreadAttributeList(HANDLE *handlesToInherit,
                                                       int handleCount) {
  if (!InitializeProcThreadAttributeListPtr ||
      !DeleteProcThreadAttributeListPtr ||
      !UpdateProcThreadAttributePtr)
    LoadThreadAttributeFunctions();
  
  if (!InitializeProcThreadAttributeListPtr ||
      !DeleteProcThreadAttributeListPtr ||
      !UpdateProcThreadAttributePtr)
    return NULL;

  SIZE_T threadAttrSize;
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;

  if (!(*InitializeProcThreadAttributeListPtr)(NULL, 1, 0, &threadAttrSize) &&
      GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    goto fail;
  lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>
                              (malloc(threadAttrSize));
  if (!lpAttributeList ||
      !(*InitializeProcThreadAttributeListPtr)(lpAttributeList, 1, 0, &threadAttrSize))
    goto fail;

  if (!(*UpdateProcThreadAttributePtr)(lpAttributeList,
                  0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                  handlesToInherit,
                  sizeof(handlesToInherit[0]) * handleCount,
                  NULL, NULL)) {
    (*DeleteProcThreadAttributeListPtr)(lpAttributeList);
    goto fail;
  }
  return lpAttributeList;

fail:
  if (lpAttributeList)
    free(lpAttributeList);
  return NULL;
}


void FreeThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList) {
  
  
  (*DeleteProcThreadAttributeListPtr)(lpAttributeList);
  free(lpAttributeList);
}

bool LaunchApp(const std::wstring& cmdline,
               bool wait, bool start_hidden, ProcessHandle* process_handle) {

  
  
  
  
  
  
  
  
  
  DWORD dwCreationFlags = 0;
  BOOL bInheritHandles = FALSE;
  
  
  STARTUPINFOEX startup_info_ex;
  ZeroMemory(&startup_info_ex, sizeof(startup_info_ex));
  STARTUPINFO &startup_info = startup_info_ex.StartupInfo;
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESHOWWINDOW;
  startup_info.wShowWindow = start_hidden ? SW_HIDE : SW_SHOW;

  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList = NULL;
  
  if (win_util::GetWinVersion() >= win_util::WINVERSION_VISTA) {
    
    
    HANDLE handlesToInherit[2];
    int handleCount = 0;
    HANDLE stdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE stdErr = ::GetStdHandle(STD_ERROR_HANDLE);

    if (IsInheritableHandle(stdOut))
      handlesToInherit[handleCount++] = stdOut;
    if (stdErr != stdOut && IsInheritableHandle(stdErr))
      handlesToInherit[handleCount++] = stdErr;

    if (handleCount)
      lpAttributeList = CreateThreadAttributeList(handlesToInherit, handleCount);
  }

  if (lpAttributeList) {
    
    startup_info.cb = sizeof(startup_info_ex);
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    startup_info.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    startup_info.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
    startup_info.hStdInput = INVALID_HANDLE_VALUE;
    startup_info_ex.lpAttributeList = lpAttributeList;
    dwCreationFlags |= EXTENDED_STARTUPINFO_PRESENT;
    bInheritHandles = TRUE;
  }
  PROCESS_INFORMATION process_info;
  BOOL createdOK = CreateProcess(NULL,
                     const_cast<wchar_t*>(cmdline.c_str()), NULL, NULL,
                     bInheritHandles, dwCreationFlags, NULL, NULL,
                     &startup_info, &process_info);
  if (lpAttributeList)
    FreeThreadAttributeList(lpAttributeList);
  if (!createdOK)
    return false;

  gProcessLog.print("==> process %d launched child process %d (%S)\n",
                    GetCurrentProcId(),
                    process_info.dwProcessId,
                    cmdline.c_str());

  
  CloseHandle(process_info.hThread);

  if (wait)
    WaitForSingleObject(process_info.hProcess, INFINITE);

  
  if (process_handle) {
    *process_handle = process_info.hProcess;
  } else {
    CloseHandle(process_info.hProcess);
  }
  return true;
}

bool LaunchApp(const CommandLine& cl,
               bool wait, bool start_hidden, ProcessHandle* process_handle) {
  return LaunchApp(cl.command_line_string(), wait,
                   start_hidden, process_handle);
}

bool KillProcess(ProcessHandle process, int exit_code, bool wait) {
  bool result = (TerminateProcess(process, exit_code) != FALSE);
  if (result && wait) {
    
    if (WAIT_OBJECT_0 != WaitForSingleObject(process, 60 * 1000))
      DLOG(ERROR) << "Error waiting for process exit: " << GetLastError();
  } else if (!result) {
    DLOG(ERROR) << "Unable to terminate process: " << GetLastError();
  }
  return result;
}

bool DidProcessCrash(bool* child_exited, ProcessHandle handle) {
  DWORD exitcode = 0;

  if (child_exited)
    *child_exited = true;  
                           
  if (!::GetExitCodeProcess(handle, &exitcode)) {
    NOTREACHED();
    return false;
  }
  if (exitcode == STILL_ACTIVE) {
    
    NOTREACHED();
    return false;
  }

  
  

  if (exitcode == PROCESS_END_NORMAL_TERMINATON ||
      exitcode == PROCESS_END_KILLED_BY_USER ||
      exitcode == PROCESS_END_PROCESS_WAS_HUNG ||
      exitcode == 0xC0000354 ||     
      exitcode == 0xC000013A ||     
      exitcode == 0x40010004) {     
    return false;
  }

  return true;
}

void SetCurrentProcessPrivileges(ChildPrivileges privs) {

}




ProcessMetrics::ProcessMetrics(ProcessHandle process) : process_(process),
                                                        last_time_(0),
                                                        last_system_time_(0) {
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  processor_count_ = system_info.dwNumberOfProcessors;
}


ProcessMetrics* ProcessMetrics::CreateProcessMetrics(ProcessHandle process) {
  return new ProcessMetrics(process);
}

ProcessMetrics::~ProcessMetrics() { }

static uint64_t FileTimeToUTC(const FILETIME& ftime) {
  LARGE_INTEGER li;
  li.LowPart = ftime.dwLowDateTime;
  li.HighPart = ftime.dwHighDateTime;
  return li.QuadPart;
}

int ProcessMetrics::GetCPUUsage() {
  FILETIME now;
  FILETIME creation_time;
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;

  GetSystemTimeAsFileTime(&now);

  if (!GetProcessTimes(process_, &creation_time, &exit_time,
                       &kernel_time, &user_time)) {
    
    
    
    return 0;
  }
  int64_t system_time = (FileTimeToUTC(kernel_time) + FileTimeToUTC(user_time)) /
                        processor_count_;
  int64_t time = FileTimeToUTC(now);

  if ((last_system_time_ == 0) || (last_time_ == 0)) {
    
    last_system_time_ = system_time;
    last_time_ = time;
    return 0;
  }

  int64_t system_time_delta = system_time - last_system_time_;
  int64_t time_delta = time - last_time_;
  DCHECK(time_delta != 0);
  if (time_delta == 0)
    return 0;

  
  int cpu = static_cast<int>((system_time_delta * 100 + time_delta / 2) /
                             time_delta);

  last_system_time_ = system_time;
  last_time_ = time;

  return cpu;
}

}  
