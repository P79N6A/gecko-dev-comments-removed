




























#ifndef CLIENT_WINDOWS_CRASH_GENERATION_CLIENT_INFO_H__
#define CLIENT_WINDOWS_CRASH_GENERATION_CLIENT_INFO_H__

#include <windows.h>
#include <dbghelp.h>
#include "client/windows/common/ipc_protocol.h"
#include "common/scoped_ptr.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {

class CrashGenerationServer;


class ClientInfo {
 public:
  
  
  
  ClientInfo(CrashGenerationServer* crash_server,
             DWORD pid,
             MINIDUMP_TYPE dump_type,
             DWORD* thread_id,
             EXCEPTION_POINTERS** ex_info,
             MDRawAssertionInfo* assert_info,
             const CustomClientInfo& custom_client_info);

  ~ClientInfo();

  CrashGenerationServer* crash_server() const { return crash_server_; }
  DWORD pid() const { return pid_; }
  MINIDUMP_TYPE dump_type() const { return dump_type_; }
  EXCEPTION_POINTERS** ex_info() const { return ex_info_; }
  MDRawAssertionInfo* assert_info() const { return assert_info_; }
  DWORD* thread_id() const { return thread_id_; }
  HANDLE process_handle() const { return process_handle_; }
  HANDLE dump_requested_handle() const { return dump_requested_handle_; }
  HANDLE dump_generated_handle() const { return dump_generated_handle_; }
  DWORD crash_id() const { return crash_id_; }

  void set_dump_request_wait_handle(HANDLE value) {
    dump_request_wait_handle_ = value;
  }

  void set_process_exit_wait_handle(HANDLE value) {
    process_exit_wait_handle_ = value;
  }

  
  
  void UnregisterDumpRequestWaitAndBlockUntilNoPending();

  
  
  
  void UnregisterProcessExitWait(bool block_until_no_pending);

  bool Initialize();
  bool GetClientExceptionInfo(EXCEPTION_POINTERS** ex_info) const;
  bool GetClientThreadId(DWORD* thread_id) const;

  
  bool PopulateCustomInfo();

  
  CustomClientInfo GetCustomInfo() const;

 private:
  
  
  void SetProcessUptime();

  
  CrashGenerationServer* crash_server_;

  
  DWORD pid_;

  
  MINIDUMP_TYPE dump_type_;

  
  
  
  
  
  
  EXCEPTION_POINTERS** ex_info_;

  
  
  
  
  
  
  
  MDRawAssertionInfo* assert_info_;

  
  CustomClientInfo custom_client_info_;

  
  
  
  scoped_array<CustomInfoEntry> custom_info_entries_;

  
  
  
  
  
  DWORD* thread_id_;

  
  HANDLE process_handle_;

  
  HANDLE dump_requested_handle_;

  
  HANDLE dump_generated_handle_;

  
  HANDLE dump_request_wait_handle_;

  
  HANDLE process_exit_wait_handle_;

  
  
  FILETIME start_time_;

  
  
  
  DWORD crash_id_;

  
  ClientInfo(const ClientInfo& client_info);
  ClientInfo& operator=(const ClientInfo& client_info);
};

}  

#endif  
