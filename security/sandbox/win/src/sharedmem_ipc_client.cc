



#include <string.h>
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/crosscall_params.h"
#include "base/logging.h"

namespace sandbox {




void* SharedMemIPCClient::GetBuffer() {
  bool failure = false;
  size_t ix = LockFreeChannel(&failure);
  if (failure) {
    return NULL;
  }
  return reinterpret_cast<char*>(control_) +
         control_->channels[ix].channel_base;
}




void SharedMemIPCClient::FreeBuffer(void* buffer) {
  size_t num = ChannelIndexFromBuffer(buffer);
  ChannelControl* channel = control_->channels;
  LONG result = ::InterlockedExchange(&channel[num].state, kFreeChannel);
  DCHECK(kFreeChannel != result);
  result;
}




SharedMemIPCClient::SharedMemIPCClient(void* shared_mem)
    : control_(reinterpret_cast<IPCControl*>(shared_mem)) {
  first_base_ = reinterpret_cast<char*>(shared_mem) +
               control_->channels[0].channel_base;
  
  DCHECK(0 != control_->channels_count);
}




ResultCode SharedMemIPCClient::DoCall(CrossCallParams* params,
                                      CrossCallReturn* answer) {
  if (!control_->server_alive)
    return SBOX_ERROR_CHANNEL_ERROR;

  size_t num = ChannelIndexFromBuffer(params->GetBuffer());
  ChannelControl* channel = control_->channels;
  
  
  
  channel[num].ipc_tag = params->GetTag();

  
  
  

  
  
  DWORD wait = ::SignalObjectAndWait(channel[num].ping_event,
                                     channel[num].pong_event,
                                     kIPCWaitTimeOut1, FALSE);
  if (WAIT_TIMEOUT == wait) {
    
    
    
    while (true) {
      wait = ::WaitForSingleObject(control_->server_alive, 0);
      if (WAIT_TIMEOUT == wait) {
        
        wait = ::WaitForSingleObject(channel[num].pong_event, kIPCWaitTimeOut1);
        if (WAIT_OBJECT_0 == wait) {
          
          break;
        } else if (WAIT_TIMEOUT == wait) {
          continue;
        } else {
          return SBOX_ERROR_CHANNEL_ERROR;
        }
      } else {
        
        
        ::InterlockedExchange(&channel[num].state, kAbandonnedChannel);
        control_->server_alive = 0;
        return SBOX_ERROR_CHANNEL_ERROR;
      }
    }
  } else if (WAIT_OBJECT_0 != wait) {
    
    return SBOX_ERROR_CHANNEL_ERROR;
  }

  
  memcpy(answer, params->GetCallReturn(), sizeof(CrossCallReturn));

  
  
  
  return answer->call_outcome;
}






size_t SharedMemIPCClient::LockFreeChannel(bool* severe_failure) {
  if (0 == control_->channels_count) {
    *severe_failure = true;
    return 0;
  }
  ChannelControl* channel = control_->channels;
  do {
    for (size_t ix = 0; ix != control_->channels_count; ++ix) {
      if (kFreeChannel == ::InterlockedCompareExchange(&channel[ix].state,
                                                       kBusyChannel,
                                                       kFreeChannel)) {
          *severe_failure = false;
          return ix;
      }
    }
    
    DWORD wait = ::WaitForSingleObject(control_->server_alive,
                                       kIPCWaitTimeOut2);
    if (WAIT_TIMEOUT != wait) {
      
      *severe_failure = true;
      return 0;
    }
  }
  while (true);
}


size_t SharedMemIPCClient::ChannelIndexFromBuffer(const void* buffer) {
  ptrdiff_t d = reinterpret_cast<const char*>(buffer) - first_base_;
  size_t num = d/kIPCChannelSize;
  DCHECK(num < control_->channels_count);
  return (num);
}

}  
