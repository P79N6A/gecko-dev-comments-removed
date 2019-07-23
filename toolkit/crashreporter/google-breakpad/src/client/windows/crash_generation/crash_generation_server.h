




























#ifndef CLIENT_WINDOWS_CRASH_GENERATION_CRASH_GENERATION_SERVER_H__
#define CLIENT_WINDOWS_CRASH_GENERATION_CRASH_GENERATION_SERVER_H__

#include <list>
#include <string>
#include "client/windows/common/ipc_protocol.h"
#include "client/windows/crash_generation/client_info.h"
#include "client/windows/crash_generation/minidump_generator.h"
#include "processor/scoped_ptr.h"

namespace google_breakpad {









class CrashGenerationServer {
 public:
  typedef void (*OnClientConnectedCallback)(void* context,
                                            const ClientInfo* client_info);

  typedef void (*OnClientDumpRequestCallback)(void* context,
                                              const ClientInfo* client_info,
                                              const std::wstring* file_path);

  typedef void (*OnClientExitedCallback)(void* context,
                                         const ClientInfo* client_info);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CrashGenerationServer(const std::wstring& pipe_name,
                        SECURITY_ATTRIBUTES* pipe_sec_attrs,
                        OnClientConnectedCallback connect_callback,
                        void* connect_context,
                        OnClientDumpRequestCallback dump_callback,
                        void* dump_context,
                        OnClientExitedCallback exit_callback,
                        void* exit_context,
                        bool generate_dumps,
                        const std::wstring* dump_path);

  ~CrashGenerationServer();

  
  
  
  bool Start();

 private:
  
  
  enum IPCServerState {
    
    IPC_SERVER_STATE_ERROR,

    
    IPC_SERVER_STATE_INITIAL,

    
    
    IPC_SERVER_STATE_CONNECTING,

    
    IPC_SERVER_STATE_CONNECTED,

    
    
    IPC_SERVER_STATE_READING,

    
    IPC_SERVER_STATE_READ_DONE,

    
    
    IPC_SERVER_STATE_WRITING,

    
    IPC_SERVER_STATE_WRITE_DONE,

    
    
    IPC_SERVER_STATE_READING_ACK,

    
    
    IPC_SERVER_STATE_DISCONNECTING
  };

  
  
  
  void HandleErrorState();
  void HandleInitialState();
  void HandleConnectingState();
  void HandleConnectedState();
  void HandleReadingState();
  void HandleReadDoneState();
  void HandleWritingState();
  void HandleWriteDoneState();
  void HandleReadingAckState();
  void HandleDisconnectingState();

  
  bool PrepareReply(const ClientInfo& client_info,
                    ProtocolMessage* reply) const;

  
  
  
  
  bool CreateClientHandles(const ClientInfo& client_info,
                           ProtocolMessage* reply) const;

  
  
  bool RespondToClient(ClientInfo* client_info);

  
  void HandleConnectionRequest();

  
  void HandleDumpRequest(const ClientInfo& client_info);

  
  static void CALLBACK OnPipeConnected(void* context, BOOLEAN timer_or_wait);

  
  static void CALLBACK OnDumpRequest(void* context, BOOLEAN timer_or_wait);

  
  static void CALLBACK OnClientEnd(void* context, BOOLEAN timer_or_wait);

  
  static DWORD WINAPI CleanupClient(void* context);

  
  void DoCleanup(ClientInfo* client_info);

  
  bool AddClient(ClientInfo* client_info);

  
  bool GenerateDump(const ClientInfo& client, std::wstring* dump_path);

  
  CRITICAL_SECTION clients_sync_;

  
  std::list<ClientInfo*> clients_;

  
  std::wstring pipe_name_;

  
  SECURITY_ATTRIBUTES* pipe_sec_attrs_;

  
  HANDLE pipe_;

  
  HANDLE pipe_wait_handle_;

  
  HANDLE server_alive_handle_;

  
  OnClientConnectedCallback connect_callback_;

  
  void* connect_context_;

  
  OnClientDumpRequestCallback dump_callback_;

  
  void* dump_context_;

  
  OnClientExitedCallback exit_callback_;

  
  void* exit_context_;

  
  bool generate_dumps_;

  
  scoped_ptr<MinidumpGenerator> dump_generator_;

  
  
  
  
  volatile IPCServerState server_state_;

  
  volatile bool shutting_down_;

  
  OVERLAPPED overlapped_;

  
  ProtocolMessage msg_;

  
  ClientInfo* client_info_;

  
  
  volatile LONG cleanup_item_count_;

  
  CrashGenerationServer(const CrashGenerationServer& crash_server);
  CrashGenerationServer& operator=(const CrashGenerationServer& crash_server);
};

}  

#endif
