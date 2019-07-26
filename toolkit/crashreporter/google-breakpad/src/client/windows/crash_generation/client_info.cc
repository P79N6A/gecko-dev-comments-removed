




























#include "client/windows/crash_generation/client_info.h"
#include "client/windows/common/ipc_protocol.h"

static const wchar_t kCustomInfoProcessUptimeName[] = L"ptime";
static const size_t kMaxCustomInfoEntries = 4096;

namespace google_breakpad {

ClientInfo::ClientInfo(CrashGenerationServer* crash_server,
                       DWORD pid,
                       MINIDUMP_TYPE dump_type,
                       DWORD* thread_id,
                       EXCEPTION_POINTERS** ex_info,
                       MDRawAssertionInfo* assert_info,
                       const CustomClientInfo& custom_client_info)
    : crash_server_(crash_server),
      pid_(pid),
      dump_type_(dump_type),
      ex_info_(ex_info),
      assert_info_(assert_info),
      custom_client_info_(custom_client_info),
      thread_id_(thread_id),
      process_handle_(NULL),
      dump_requested_handle_(NULL),
      dump_generated_handle_(NULL),
      dump_request_wait_handle_(NULL),
      process_exit_wait_handle_(NULL),
      crash_id_(NULL) {
  GetSystemTimeAsFileTime(&start_time_);
}

bool ClientInfo::Initialize() {
  process_handle_ = OpenProcess(GENERIC_ALL, FALSE, pid_);
  if (!process_handle_) {
    return false;
  }

  
  FILETIME creation_time, exit_time, kernel_time, user_time;
  if (GetProcessTimes(process_handle_, &creation_time, &exit_time,
                      &kernel_time, &user_time))
    crash_id_ = creation_time.dwLowDateTime;

  dump_requested_handle_ = CreateEvent(NULL,    
                                       TRUE,    
                                       FALSE,   
                                       NULL);   
  if (!dump_requested_handle_) {
    return false;
  }

  dump_generated_handle_ = CreateEvent(NULL,    
                                       TRUE,    
                                       FALSE,   
                                       NULL);   
  return dump_generated_handle_ != NULL;
}

void ClientInfo::UnregisterDumpRequestWaitAndBlockUntilNoPending() {
  if (dump_request_wait_handle_) {
    
    UnregisterWaitEx(dump_request_wait_handle_, INVALID_HANDLE_VALUE);
    dump_request_wait_handle_ = NULL;
  }
}

void ClientInfo::UnregisterProcessExitWait(bool block_until_no_pending) {
  if (process_exit_wait_handle_) {
    if (block_until_no_pending) {
      
      UnregisterWaitEx(process_exit_wait_handle_, INVALID_HANDLE_VALUE);
    } else {
      UnregisterWait(process_exit_wait_handle_);
    }
    process_exit_wait_handle_ = NULL;
  }
}

ClientInfo::~ClientInfo() {
  
  
  UnregisterDumpRequestWaitAndBlockUntilNoPending();

  
  
  
  
  
  
  UnregisterProcessExitWait(true);

  if (process_handle_) {
    CloseHandle(process_handle_);
  }

  if (dump_requested_handle_) {
    CloseHandle(dump_requested_handle_);
  }

  if (dump_generated_handle_) {
    CloseHandle(dump_generated_handle_);
  }
}

bool ClientInfo::GetClientExceptionInfo(EXCEPTION_POINTERS** ex_info) const {
  SIZE_T bytes_count = 0;
  if (!ReadProcessMemory(process_handle_,
                         ex_info_,
                         ex_info,
                         sizeof(*ex_info),
                         &bytes_count)) {
    return false;
  }

  return bytes_count == sizeof(*ex_info);
}

bool ClientInfo::GetClientThreadId(DWORD* thread_id) const {
  SIZE_T bytes_count = 0;
  if (!ReadProcessMemory(process_handle_,
                         thread_id_,
                         thread_id,
                         sizeof(*thread_id),
                         &bytes_count)) {
    return false;
  }

  return bytes_count == sizeof(*thread_id);
}

void ClientInfo::SetProcessUptime() {
  FILETIME now = {0};
  GetSystemTimeAsFileTime(&now);

  ULARGE_INTEGER time_start;
  time_start.HighPart = start_time_.dwHighDateTime;
  time_start.LowPart = start_time_.dwLowDateTime;

  ULARGE_INTEGER time_now;
  time_now.HighPart = now.dwHighDateTime;
  time_now.LowPart = now.dwLowDateTime;

  
  __int64 delay = (time_now.QuadPart - time_start.QuadPart) / 10 / 1000;

  
  wchar_t* value = custom_info_entries_.get()[custom_client_info_.count].value;
  _i64tow_s(delay, value, CustomInfoEntry::kValueMaxLength, 10);
}

bool ClientInfo::PopulateCustomInfo() {
  if (custom_client_info_.count > kMaxCustomInfoEntries)
    return false;

  SIZE_T bytes_count = 0;
  SIZE_T read_count = sizeof(CustomInfoEntry) * custom_client_info_.count;

  
  
  
  
  if (!custom_info_entries_.get()) {
    
    custom_info_entries_.reset(
        new CustomInfoEntry[custom_client_info_.count + 1]);
    
    custom_info_entries_.get()[custom_client_info_.count].set_name(
        kCustomInfoProcessUptimeName);
  }

  if (!ReadProcessMemory(process_handle_,
                         custom_client_info_.entries,
                         custom_info_entries_.get(),
                         read_count,
                         &bytes_count)) {
    return false;
  }

  SetProcessUptime();
  return (bytes_count != read_count);
}

CustomClientInfo ClientInfo::GetCustomInfo() const {
  CustomClientInfo custom_info;
  custom_info.entries = custom_info_entries_.get();
  
  
  custom_info.count = custom_client_info_.count + 1;
  return custom_info;
}

}  
