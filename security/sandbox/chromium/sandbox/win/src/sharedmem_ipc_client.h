



#ifndef SANDBOX_SRC_SHAREDMEM_IPC_CLIENT_H__
#define SANDBOX_SRC_SHAREDMEM_IPC_CLIENT_H__

#include "sandbox/win/src/crosscall_params.h"
#include "sandbox/win/src/sandbox.h"










































namespace sandbox {


enum ChannelState {
  
  kFreeChannel = 1,
  
  kBusyChannel,
  
  kAckChannel,
  
  kReadyChannel,
  
  kAbandonnedChannel
};


const DWORD kIPCWaitTimeOut1 = 1000;   
const DWORD kIPCWaitTimeOut2 =   50;   


struct ChannelControl {
  
  size_t channel_base;
  
  volatile LONG state;
  
  
  HANDLE ping_event;
  
  HANDLE pong_event;
  
  uint32 ipc_tag;
};

struct IPCControl {
  
  size_t channels_count;
  
  HANDLE server_alive;
  
  ChannelControl channels[1];
};




class SharedMemIPCClient {
 public:
  
  
  explicit SharedMemIPCClient(void* shared_mem);

  
  
  void* GetBuffer();

  
  
  void FreeBuffer(void* buffer);

  
  
  
  
  
  
  
  ResultCode DoCall(CrossCallParams* params, CrossCallReturn* answer);

 private:
  
  
  
  size_t LockFreeChannel(bool* severe_failure);
  
  size_t ChannelIndexFromBuffer(const void* buffer);
  IPCControl* control_;
  
  char* first_base_;
};

}  

#endif  
