




























#ifndef CLIENT_LINUX_CRASH_GENERATION_CRASH_GENERATION_SERVER_H_
#define CLIENT_LINUX_CRASH_GENERATION_CRASH_GENERATION_SERVER_H_

#include <pthread.h>

#include <string>

#include "common/using_std_string.h"

namespace google_breakpad {

class ClientInfo;

class CrashGenerationServer {
public:
  
  
  
  typedef void (*OnClientDumpRequestCallback)(void* context,
                                              const ClientInfo* client_info,
                                              const string* file_path);

  typedef void (*OnClientExitingCallback)(void* context,
                                          const ClientInfo* client_info);

  
  
  
  
  
  
  
  
  
  
  
  
  
  CrashGenerationServer(const int listen_fd,
                        OnClientDumpRequestCallback dump_callback,
                        void* dump_context,
                        OnClientExitingCallback exit_callback,
                        void* exit_context,
                        bool generate_dumps,
                        const string* dump_path);

  ~CrashGenerationServer();

  
  
  
  bool Start();

  
  void Stop();

  
  
  
  
  static bool CreateReportChannel(int* server_fd, int* client_fd);

private:
  
  void Run();

  
  
  bool ClientEvent(short revents);

  
  
  bool ControlEvent(short revents);

  
  bool MakeMinidumpFilename(string& outFilename);

  
  static void* ThreadMain(void* arg);

  int server_fd_;

  OnClientDumpRequestCallback dump_callback_;
  void* dump_context_;

  OnClientExitingCallback exit_callback_;
  void* exit_context_;

  bool generate_dumps_;

  string dump_dir_;

  bool started_;

  pthread_t thread_;
  int control_pipe_in_;
  int control_pipe_out_;

  
  CrashGenerationServer(const CrashGenerationServer&);
  CrashGenerationServer& operator=(const CrashGenerationServer&);
};

} 

#endif 
