



#ifndef SANDBOX_SRC_SHAREDMEM_IPC_SERVER_H_
#define SANDBOX_SRC_SHAREDMEM_IPC_SERVER_H_

#include <list>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "sandbox/win/src/crosscall_params.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"

















namespace sandbox {



class SharedMemIPCServer {
 public:
  
  
  
  
  
  
  SharedMemIPCServer(HANDLE target_process, DWORD target_process_id,
                     HANDLE target_job, ThreadProvider* thread_provider,
                     Dispatcher* dispatcher);

  ~SharedMemIPCServer();

  
  
  bool Init(void* shared_mem, uint32 shared_size, uint32 channel_size);

 private:
  
  
  FRIEND_TEST_ALL_PREFIXES(IPCTest, SharedMemServerTests);
  
  
  
  static void __stdcall ThreadPingEventReady(void* context,
                                             unsigned char);

  
  
  bool MakeEvents(HANDLE* server_ping, HANDLE* server_pong,
                  HANDLE* client_ping, HANDLE* client_pong);

  
  
  
  
  
  struct ServerControl {
    
    HANDLE ping_event;
    
    HANDLE pong_event;
    
    uint32 channel_size;
    
    char* channel_buffer;
    
    char* shared_base;
    
    
    ChannelControl* channel;
    
    Dispatcher* dispatcher;
    
    ClientInfo target_info;
  };

  
  static bool InvokeCallback(const ServerControl* service_context,
                             void* ipc_buffer, CrossCallReturn* call_result);

  
  
  IPCControl* client_control_;

  
  typedef std::list<ServerControl*> ServerContexts;
  ServerContexts server_contexts_;

  
  
  ThreadProvider* thread_provider_;

  
  HANDLE target_process_;

  
  DWORD target_process_id_;

  
  HANDLE target_job_object_;

  
  Dispatcher* call_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(SharedMemIPCServer);
};

}  

#endif  
