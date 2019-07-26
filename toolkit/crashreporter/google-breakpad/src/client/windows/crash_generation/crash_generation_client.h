




























#ifndef CLIENT_WINDOWS_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_
#define CLIENT_WINDOWS_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_

#include <windows.h>
#include <dbghelp.h>
#include <string>
#include <utility>
#include "client/windows/common/ipc_protocol.h"
#include "common/scoped_ptr.h"

namespace google_breakpad {

struct CustomClientInfo;




















class CrashGenerationClient {
 public:
  CrashGenerationClient(const wchar_t* pipe_name,
                        MINIDUMP_TYPE dump_type,
                        const CustomClientInfo* custom_info);

  CrashGenerationClient(HANDLE pipe_handle,
                        MINIDUMP_TYPE dump_type,
                        const CustomClientInfo* custom_info);

  ~CrashGenerationClient();

  
  
  
  bool Register();

  
  
  bool RequestUpload(DWORD crash_id);

  bool RequestDump(EXCEPTION_POINTERS* ex_info,
                   MDRawAssertionInfo* assert_info);

  
  
  
  
  
  
  bool RequestDump(EXCEPTION_POINTERS* ex_info);

  
  
  
  
  
  
  bool RequestDump(MDRawAssertionInfo* assert_info);

  
  
  
  
  
  static HANDLE DuplicatePipeToClientProcess(const wchar_t* pipe_name,
                                             HANDLE hProcess);

 private:
  
  
  
  HANDLE ConnectToServer();

  
  
  
  
  bool RegisterClient(HANDLE pipe);

  
  bool ValidateResponse(const ProtocolMessage& msg) const;

  
  bool IsRegistered() const;

  
  
  
  HANDLE ConnectToPipe(const wchar_t* pipe_name,
                       DWORD pipe_access,
                       DWORD flags_attrs);

  
  bool SignalCrashEventAndWait();

  
  std::wstring pipe_name_;

  
  
  HANDLE pipe_handle_;

  
  CustomClientInfo custom_info_;

  
  MINIDUMP_TYPE dump_type_;

  
  HANDLE crash_event_;

  
  
  HANDLE crash_generated_;

  
  
  HANDLE server_alive_;

  
  DWORD server_process_id_;

  
  DWORD thread_id_;

  
  EXCEPTION_POINTERS* exception_pointers_;

  
  MDRawAssertionInfo assert_info_;

  
  CrashGenerationClient(const CrashGenerationClient& crash_client);
  CrashGenerationClient& operator=(const CrashGenerationClient& crash_client);
};

}  

#endif  
