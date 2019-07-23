



#include "base/process_util.h"

#include <windows.h>
#include <winternl.h>
#include <psapi.h>

#include "base/histogram.h"
#include "base/logging.h"
#include "base/scoped_handle_win.h"
#include "base/scoped_ptr.h"

namespace {


const int PAGESIZE_KB = 4;


typedef BOOL (WINAPI* HeapSetFn)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);

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
  CloseHandle(process);
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

bool LaunchApp(const std::wstring& cmdline,
               bool wait, bool start_hidden, ProcessHandle* process_handle) {
  STARTUPINFO startup_info = {0};
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESHOWWINDOW;
  startup_info.wShowWindow = start_hidden ? SW_HIDE : SW_SHOW;
  PROCESS_INFORMATION process_info;
  if (!CreateProcess(NULL,
                     const_cast<wchar_t*>(cmdline.c_str()), NULL, NULL,
                     FALSE, 0, NULL, NULL,
                     &startup_info, &process_info))
    return false;

  
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




bool KillProcessById(ProcessId process_id, int exit_code, bool wait) {
  HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE,
                               FALSE,  
                               process_id);
  if (!process)
    return false;

  bool ret = KillProcess(process, exit_code, wait);
  CloseHandle(process);
  return ret;
}

bool GetAppOutput(const std::wstring& cmd_line, std::string* output) {
  if (!output) {
    NOTREACHED();
    return false;
  }

  HANDLE out_read = NULL;
  HANDLE out_write = NULL;

  SECURITY_ATTRIBUTES sa_attr;
  
  sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa_attr.bInheritHandle = TRUE;
  sa_attr.lpSecurityDescriptor = NULL;

  
  if (!CreatePipe(&out_read, &out_write, &sa_attr, 0)) {
    NOTREACHED() << "Failed to create pipe";
    return false;
  }

  
  ScopedHandle scoped_out_read(out_read);
  ScopedHandle scoped_out_write(out_write);

  
  if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0)) {
    NOTREACHED() << "Failed to disabled pipe inheritance";
    return false;
  }

  
  PROCESS_INFORMATION proc_info = { 0 };
  STARTUPINFO start_info = { 0 };

  start_info.cb = sizeof(STARTUPINFO);
  start_info.hStdOutput = out_write;
  
  start_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  start_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  start_info.dwFlags |= STARTF_USESTDHANDLES;

  
  if (!CreateProcess(NULL, const_cast<wchar_t*>(cmd_line.c_str()), NULL, NULL,
                     TRUE,  
                     0, NULL, NULL, &start_info, &proc_info)) {
    NOTREACHED() << "Failed to start process";
    return false;
  }

  
  CloseHandle(proc_info.hThread);

  
  
  scoped_out_write.Close();

  
  const int kBufferSize = 1024;
  char buffer[kBufferSize];

  for (;;) {
    DWORD bytes_read = 0;
    BOOL success = ReadFile(out_read, buffer, kBufferSize, &bytes_read, NULL);
    if (!success || bytes_read == 0)
      break;
    output->append(buffer, bytes_read);
  }

  
  WaitForSingleObject(proc_info.hProcess, INFINITE);
  CloseHandle(proc_info.hProcess);

  return true;
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

  

  
  
  
  const int kLeastValue = 0;
  const int kMaxValue = 0xFFF;
  const int kBucketCount = kMaxValue - kLeastValue + 1;
  static LinearHistogram least_significant_histogram("ExitCodes.LSNibbles",
      kLeastValue + 1, kMaxValue, kBucketCount);
  least_significant_histogram.SetFlags(kUmaTargetedHistogramFlag |
                                       LinearHistogram::kHexRangePrintingFlag);
  least_significant_histogram.Add(exitcode & 0xFFF);

  
  static LinearHistogram most_significant_histogram("ExitCodes.MSNibbles",
      kLeastValue + 1, kMaxValue, kBucketCount);
  most_significant_histogram.SetFlags(kUmaTargetedHistogramFlag |
                                      LinearHistogram::kHexRangePrintingFlag);
  
  most_significant_histogram.Add((exitcode >> 20) & 0xFFF);

  
  static LinearHistogram mid_significant_histogram("ExitCodes.MidNibbles",
      1, 0xFF, 0x100);
  mid_significant_histogram.SetFlags(kUmaTargetedHistogramFlag |
                                      LinearHistogram::kHexRangePrintingFlag);
  mid_significant_histogram.Add((exitcode >> 12) & 0xFF);

  return true;
}

bool WaitForExitCode(ProcessHandle handle, int* exit_code) {
  ScopedHandle closer(handle);  
  if (::WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0) {
    NOTREACHED();
    return false;
  }
  DWORD temp_code;  
  if (!::GetExitCodeProcess(handle, &temp_code))
    return false;
  *exit_code = temp_code;
  return true;
}

NamedProcessIterator::NamedProcessIterator(const std::wstring& executable_name,
                                           const ProcessFilter* filter)
    : started_iteration_(false),
      executable_name_(executable_name),
      filter_(filter) {
  snapshot_ = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
}

NamedProcessIterator::~NamedProcessIterator() {
  CloseHandle(snapshot_);
}


const ProcessEntry* NamedProcessIterator::NextProcessEntry() {
  bool result = false;
  do {
    result = CheckForNextProcess();
  } while (result && !IncludeEntry());

  if (result) {
    return &entry_;
  }

  return NULL;
}

bool NamedProcessIterator::CheckForNextProcess() {
  InitProcessEntry(&entry_);

  if (!started_iteration_) {
    started_iteration_ = true;
    return !!Process32First(snapshot_, &entry_);
  }

  return !!Process32Next(snapshot_, &entry_);
}

bool NamedProcessIterator::IncludeEntry() {
  return _wcsicmp(executable_name_.c_str(), entry_.szExeFile) == 0 &&
                  (!filter_ || filter_->Includes(entry_.th32ProcessID,
                                                 entry_.th32ParentProcessID));
}

void NamedProcessIterator::InitProcessEntry(ProcessEntry* entry) {
  memset(entry, 0, sizeof(*entry));
  entry->dwSize = sizeof(*entry);
}

int GetProcessCount(const std::wstring& executable_name,
                    const ProcessFilter* filter) {
  int count = 0;

  NamedProcessIterator iter(executable_name, filter);
  while (iter.NextProcessEntry())
    ++count;
  return count;
}

bool KillProcesses(const std::wstring& executable_name, int exit_code,
                   const ProcessFilter* filter) {
  bool result = true;
  const ProcessEntry* entry;

  NamedProcessIterator iter(executable_name, filter);
  while (entry = iter.NextProcessEntry()) {
    if (!KillProcessById((*entry).th32ProcessID, exit_code, true))
      result = false;
  }

  return result;
}

bool WaitForProcessesToExit(const std::wstring& executable_name,
                            int wait_milliseconds,
                            const ProcessFilter* filter) {
  const ProcessEntry* entry;
  bool result = true;
  DWORD start_time = GetTickCount();

  NamedProcessIterator iter(executable_name, filter);
  while (entry = iter.NextProcessEntry()) {
    DWORD remaining_wait =
      std::max(0, wait_milliseconds -
          static_cast<int>(GetTickCount() - start_time));
    HANDLE process = OpenProcess(SYNCHRONIZE,
                                 FALSE,
                                 entry->th32ProcessID);
    DWORD wait_result = WaitForSingleObject(process, remaining_wait);
    CloseHandle(process);
    result = result && (wait_result == WAIT_OBJECT_0);
  }

  return result;
}

bool WaitForSingleProcess(ProcessHandle handle, int wait_milliseconds) {
  bool retval = WaitForSingleObject(handle, wait_milliseconds) == WAIT_OBJECT_0;
  return retval;
}

bool CrashAwareSleep(ProcessHandle handle, int wait_milliseconds) {
  bool retval = WaitForSingleObject(handle, wait_milliseconds) == WAIT_TIMEOUT;
  return retval;
}

bool CleanupProcesses(const std::wstring& executable_name,
                      int wait_milliseconds,
                      int exit_code,
                      const ProcessFilter* filter) {
  bool exited_cleanly = WaitForProcessesToExit(executable_name,
                                               wait_milliseconds,
                                               filter);
  if (!exited_cleanly)
    KillProcesses(executable_name, exit_code, filter);
  return exited_cleanly;
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

size_t ProcessMetrics::GetPagefileUsage() const {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(process_, &pmc, sizeof(pmc))) {
    return pmc.PagefileUsage;
  }
  return 0;
}


size_t ProcessMetrics::GetPeakPagefileUsage() const {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(process_, &pmc, sizeof(pmc))) {
    return pmc.PeakPagefileUsage;
  }
  return 0;
}


size_t ProcessMetrics::GetWorkingSetSize() const {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(process_, &pmc, sizeof(pmc))) {
    return pmc.WorkingSetSize;
  }
  return 0;
}

size_t ProcessMetrics::GetPrivateBytes() const {
  
  
  
  
  PROCESS_MEMORY_COUNTERS_EX pmcx;
  if (GetProcessMemoryInfo(process_,
                          reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmcx),
                          sizeof(pmcx))) {
      return pmcx.PrivateUsage;
  }
  return 0;
}

void ProcessMetrics::GetCommittedKBytes(CommittedKBytes* usage) const {
  MEMORY_BASIC_INFORMATION mbi = {0};
  size_t committed_private = 0;
  size_t committed_mapped = 0;
  size_t committed_image = 0;
  void* base_address = NULL;
  while (VirtualQueryEx(process_, base_address, &mbi, sizeof(mbi)) ==
      sizeof(mbi)) {
    if (mbi.State == MEM_COMMIT) {
      if (mbi.Type == MEM_PRIVATE) {
        committed_private += mbi.RegionSize;
      } else if (mbi.Type == MEM_MAPPED) {
        committed_mapped += mbi.RegionSize;
      } else if (mbi.Type == MEM_IMAGE) {
        committed_image += mbi.RegionSize;
      } else {
        NOTREACHED();
      }
    }
    void* new_base = (static_cast<BYTE*>(mbi.BaseAddress)) + mbi.RegionSize;
    
    
    
    if (new_base <= base_address) {
      usage->image = 0;
      usage->mapped = 0;
      usage->priv = 0;
      return;
    }
    base_address = new_base;
  }
  usage->image = committed_image / 1024;
  usage->mapped = committed_mapped / 1024;
  usage->priv = committed_private / 1024;
}

bool ProcessMetrics::GetWorkingSetKBytes(WorkingSetKBytes* ws_usage) const {
  size_t ws_private = 0;
  size_t ws_shareable = 0;
  size_t ws_shared = 0;

  DCHECK(ws_usage);
  memset(ws_usage, 0, sizeof(*ws_usage));

  DWORD number_of_entries = 4096;  
  PSAPI_WORKING_SET_INFORMATION* buffer = NULL;
  int retries = 5;
  for (;;) {
    DWORD buffer_size = sizeof(PSAPI_WORKING_SET_INFORMATION) +
                        (number_of_entries * sizeof(PSAPI_WORKING_SET_BLOCK));

    
    
    PSAPI_WORKING_SET_INFORMATION* new_buffer =
        reinterpret_cast<PSAPI_WORKING_SET_INFORMATION*>(
            realloc(buffer, buffer_size));
    if (!new_buffer) {
      free(buffer);
      return false;
    }
    buffer = new_buffer;

    
    if (QueryWorkingSet(process_, buffer, buffer_size))
      break;  

    if (GetLastError() != ERROR_BAD_LENGTH) {
      free(buffer);
      return false;
    }

    number_of_entries = static_cast<DWORD>(buffer->NumberOfEntries);

    
    
    number_of_entries = static_cast<DWORD>(number_of_entries * 1.25);

    if (--retries == 0) {
      free(buffer);  
      return false;
    }
  }

  
  
  
  number_of_entries =
      std::min(number_of_entries, static_cast<DWORD>(buffer->NumberOfEntries));
  for (unsigned int i = 0; i < number_of_entries; i++) {
    if (buffer->WorkingSetInfo[i].Shared) {
      ws_shareable++;
      if (buffer->WorkingSetInfo[i].ShareCount > 1)
        ws_shared++;
    } else {
      ws_private++;
    }
  }

  ws_usage->priv = ws_private * PAGESIZE_KB;
  ws_usage->shareable = ws_shareable * PAGESIZE_KB;
  ws_usage->shared = ws_shared * PAGESIZE_KB;
  free(buffer);
  return true;
}

static uint64 FileTimeToUTC(const FILETIME& ftime) {
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
  int64 system_time = (FileTimeToUTC(kernel_time) + FileTimeToUTC(user_time)) /
                        processor_count_;
  int64 time = FileTimeToUTC(now);

  if ((last_system_time_ == 0) || (last_time_ == 0)) {
    
    last_system_time_ = system_time;
    last_time_ = time;
    return 0;
  }

  int64 system_time_delta = system_time - last_system_time_;
  int64 time_delta = time - last_time_;
  DCHECK(time_delta != 0);
  if (time_delta == 0)
    return 0;

  
  int cpu = static_cast<int>((system_time_delta * 100 + time_delta / 2) /
                             time_delta);

  last_system_time_ = system_time;
  last_time_ = time;

  return cpu;
}

bool ProcessMetrics::GetIOCounters(IO_COUNTERS* io_counters) const {
  return GetProcessIoCounters(process_, io_counters) != FALSE;
}

bool ProcessMetrics::CalculateFreeMemory(FreeMBytes* free) const {
  const SIZE_T kTopAdress = 0x7F000000;
  const SIZE_T kMegabyte = 1024 * 1024;
  SIZE_T accumulated = 0;

  MEMORY_BASIC_INFORMATION largest = {0};
  UINT_PTR scan = 0;
  while (scan < kTopAdress) {
    MEMORY_BASIC_INFORMATION info;
    if (!::VirtualQueryEx(process_, reinterpret_cast<void*>(scan),
                          &info, sizeof(info)))
      return false;
    if (info.State == MEM_FREE) {
      accumulated += info.RegionSize;
      UINT_PTR end = scan + info.RegionSize;
      if (info.RegionSize > (largest.RegionSize))
        largest = info;
    }
    scan += info.RegionSize;
  }
  free->largest = largest.RegionSize / kMegabyte;
  free->largest_ptr = largest.BaseAddress;
  free->total = accumulated / kMegabyte;
  return true;
}

bool EnableLowFragmentationHeap() {
  HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");
  HeapSetFn heap_set = reinterpret_cast<HeapSetFn>(GetProcAddress(
      kernel32,
      "HeapSetInformation"));

  
  
  if (!heap_set)
    return true;

  unsigned number_heaps = GetProcessHeaps(0, NULL);
  if (!number_heaps)
    return false;

  
  
  static const int MARGIN = 8;
  scoped_array<HANDLE> heaps(new HANDLE[number_heaps + MARGIN]);
  number_heaps = GetProcessHeaps(number_heaps + MARGIN, heaps.get());
  if (!number_heaps)
    return false;

  for (unsigned i = 0; i < number_heaps; ++i) {
    ULONG lfh_flag = 2;
    
    
    heap_set(heaps[i],
             HeapCompatibilityInformation,
             &lfh_flag,
             sizeof(lfh_flag));
  }
  return true;
}

void EnableTerminationOnHeapCorruption() {
  
  HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
}

void RaiseProcessToHighPriority() {
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

}  
