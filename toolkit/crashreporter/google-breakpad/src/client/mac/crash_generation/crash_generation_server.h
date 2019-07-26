




























#ifndef GOOGLE_BREAKPAD_CLIENT_MAC_CRASH_GENERATION_CRASH_GENERATION_SERVER_H_
#define GOOGLE_BREAKPAD_CLIENT_MAC_CRASH_GENERATION_CRASH_GENERATION_SERVER_H_

#include <stdint.h>

#include <string>

#include "common/mac/MachIPC.h"

namespace google_breakpad {

class ClientInfo;


enum {
  kDumpRequestMessage     = 1,
  kAcknowledgementMessage = 2,
  kQuitMessage            = 3
};


struct ExceptionInfo {
  int32_t exception_type;
  int32_t exception_code;
  int64_t exception_subcode;
};

class CrashGenerationServer {
 public:
  
  
  
  typedef void (*OnClientDumpRequestCallback)(void *context,
                                              const ClientInfo &client_info,
                                              const std::string &file_path);

  typedef void (*OnClientExitingCallback)(void *context,
                                          const ClientInfo &client_info);
  
  typedef bool (*FilterCallback)(void *context);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CrashGenerationServer(const char *mach_port_name,
                        FilterCallback filter,
                        void *filter_context,
                        OnClientDumpRequestCallback dump_callback,
                        void *dump_context,
                        OnClientExitingCallback exit_callback,
                        void *exit_context,
                        bool generate_dumps,
                        const std::string &dump_path);

  ~CrashGenerationServer();

  
  
  
  bool Start();

  
  bool Stop();

 private:
  
  bool MakeMinidumpFilename(std::string &outFilename);

  
  
  static void *WaitForMessages(void *server);

  
  
  bool WaitForOneMessage();

  FilterCallback filter_;
  void *filter_context_;

  OnClientDumpRequestCallback dump_callback_;
  void *dump_context_;

  OnClientExitingCallback exit_callback_;
  void *exit_context_;

  bool generate_dumps_;

  std::string dump_dir_;

  bool started_;

  
  ReceivePort receive_port_;

  
  
  std::string mach_port_name_;

  
  pthread_t server_thread_;

  
  CrashGenerationServer(const CrashGenerationServer&);
  CrashGenerationServer& operator=(const CrashGenerationServer&);
};

}  

#endif  
