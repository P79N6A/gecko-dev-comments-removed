




























#ifndef CLIENT_WINDOWS_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H__
#define CLIENT_WINDOWS_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H__

#include <string>
#include "client/windows/common/ipc_protocol.h"

namespace google_breakpad {




















class CrashGenerationClient {
 public:
  CrashGenerationClient(const wchar_t* pipe_name,
                        MINIDUMP_TYPE dump_type);

  ~CrashGenerationClient();

  
  
  
  bool Register();

  
  
  
  
  
  
  bool RequestDump(EXCEPTION_POINTERS* ex_info);

  
  
  
  
  
  
  bool RequestDump(MDRawAssertionInfo* assert_info);

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
