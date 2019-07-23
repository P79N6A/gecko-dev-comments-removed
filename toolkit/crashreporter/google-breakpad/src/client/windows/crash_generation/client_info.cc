




























#include "client/windows/crash_generation/client_info.h"

namespace google_breakpad {

ClientInfo::ClientInfo(CrashGenerationServer* crash_server,
                       DWORD pid,
                       MINIDUMP_TYPE dump_type,
                       DWORD* thread_id,
                       EXCEPTION_POINTERS** ex_info,
                       MDRawAssertionInfo* assert_info)
    : crash_server_(crash_server),
      pid_(pid),
      dump_type_(dump_type),
      ex_info_(ex_info),
      assert_info_(assert_info),
      thread_id_(thread_id),
      process_handle_(NULL),
      dump_requested_handle_(NULL),
      dump_generated_handle_(NULL),
      dump_request_wait_handle_(NULL),
      process_exit_wait_handle_(NULL) {
}

bool ClientInfo::Initialize() {
  process_handle_ = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid_);
  if (!process_handle_) {
    return false;
  }

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

ClientInfo::~ClientInfo() {
  if (process_handle_) {
    CloseHandle(process_handle_);
  }

  if (dump_requested_handle_) {
    CloseHandle(dump_requested_handle_);
  }

  if (dump_generated_handle_) {
    CloseHandle(dump_generated_handle_);
  }

  if (dump_request_wait_handle_) {
    
    UnregisterWaitEx(dump_request_wait_handle_, INVALID_HANDLE_VALUE);
  }

  if (process_exit_wait_handle_) {
    
    UnregisterWaitEx(process_exit_wait_handle_, INVALID_HANDLE_VALUE);
  }
}

bool ClientInfo::UnregisterWaits() {
  bool success = true;

  if (dump_request_wait_handle_) {
    if (!UnregisterWait(dump_request_wait_handle_)) {
      success = false;
    } else {
      dump_request_wait_handle_ = NULL;
    }
  }

  if (process_exit_wait_handle_) {
    if (!UnregisterWait(process_exit_wait_handle_)) {
      success = false;
    } else {
      process_exit_wait_handle_ = NULL;
    }
  }

  return success;
}

bool ClientInfo::GetClientExceptionInfo(
    EXCEPTION_POINTERS** ex_info) const {
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

}  
