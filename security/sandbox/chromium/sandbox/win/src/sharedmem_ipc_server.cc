



#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/sharedmem_ipc_server.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/crosscall_params.h"
#include "sandbox/win/src/crosscall_server.h"

namespace {

volatile HANDLE g_alive_mutex = NULL;
}

namespace sandbox {

SharedMemIPCServer::SharedMemIPCServer(HANDLE target_process,
                                       DWORD target_process_id,
                                       HANDLE target_job,
                                       ThreadProvider* thread_provider,
                                       Dispatcher* dispatcher)
    : client_control_(NULL),
      thread_provider_(thread_provider),
      target_process_(target_process),
      target_process_id_(target_process_id),
      target_job_object_(target_job),
      call_dispatcher_(dispatcher) {
  
  
  
  
  
  
  if (!g_alive_mutex) {
    HANDLE mutex = ::CreateMutexW(NULL, TRUE, NULL);
    if (::InterlockedCompareExchangePointer(&g_alive_mutex, mutex, NULL)) {
      
      ::CloseHandle(mutex);
    }
  }
}

SharedMemIPCServer::~SharedMemIPCServer() {
  
  if (!thread_provider_->UnRegisterWaits(this)) {
    
    return;
  }
  
  ServerContexts::iterator it;
  for (it = server_contexts_.begin(); it != server_contexts_.end(); ++it) {
    ServerControl* context = (*it);
    ::CloseHandle(context->ping_event);
    ::CloseHandle(context->pong_event);
    delete context;
  }
}

bool SharedMemIPCServer::Init(void* shared_mem, uint32 shared_size,
                              uint32 channel_size) {
  
  if (shared_size < channel_size) {
    return false;
  }
  
  if (0 != (channel_size % 32)) {
    return false;
  }

  
  shared_size -= offsetof(IPCControl, channels);
  size_t channel_count = shared_size / (sizeof(ChannelControl) + channel_size);

  
  if (0 == channel_count) {
    return false;
  }
  
  size_t base_start = (sizeof(ChannelControl)* channel_count) +
                       offsetof(IPCControl, channels);

  client_control_ = reinterpret_cast<IPCControl*>(shared_mem);
  client_control_->channels_count = 0;

  
  
  
  
  
  
  for (size_t ix = 0; ix != channel_count; ++ix) {
    ChannelControl* client_context = &client_control_->channels[ix];
    ServerControl* service_context = new ServerControl;
    server_contexts_.push_back(service_context);

    if (!MakeEvents(&service_context->ping_event,
                    &service_context->pong_event,
                    &client_context->ping_event,
                    &client_context->pong_event)) {
      return false;
    }

    client_context->channel_base = base_start;
    client_context->state = kFreeChannel;

    
    
    
    service_context->shared_base = reinterpret_cast<char*>(shared_mem);
    service_context->channel_size = channel_size;
    service_context->channel = client_context;
    service_context->channel_buffer = service_context->shared_base +
                                      client_context->channel_base;
    service_context->dispatcher = call_dispatcher_;
    service_context->target_info.process = target_process_;
    service_context->target_info.process_id = target_process_id_;
    service_context->target_info.job_object = target_job_object_;
    
    base_start += channel_size;
    
    thread_provider_->RegisterWait(this, service_context->ping_event,
                                   ThreadPingEventReady, service_context);
  }
  if (!::DuplicateHandle(::GetCurrentProcess(), g_alive_mutex,
                         target_process_, &client_control_->server_alive,
                         SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, 0)) {
    return false;
  }
  
  client_control_->channels_count = channel_count;
  return true;
}


void ReleaseArgs(const IPCParams* ipc_params, void* args[kMaxIpcParams]) {
  for (size_t i = 0; i < kMaxIpcParams; i++) {
    switch (ipc_params->args[i]) {
      case WCHAR_TYPE: {
        delete reinterpret_cast<base::string16*>(args[i]);
        args[i] = NULL;
        break;
      }
      case INOUTPTR_TYPE: {
        delete reinterpret_cast<CountedBuffer*>(args[i]);
        args[i] = NULL;
        break;
      }
      default: break;
    }
  }
}


bool GetArgs(CrossCallParamsEx* params, IPCParams* ipc_params,
             void* args[kMaxIpcParams]) {
  if (kMaxIpcParams < params->GetParamsCount())
    return false;

  for (uint32 i = 0; i < params->GetParamsCount(); i++) {
    uint32 size;
    ArgType type;
    args[i] = params->GetRawParameter(i, &size, &type);
    if (args[i]) {
      ipc_params->args[i] = type;
      switch (type) {
        case WCHAR_TYPE: {
          scoped_ptr<base::string16> data(new base::string16);
          if (!params->GetParameterStr(i, data.get())) {
            args[i] = 0;
            ReleaseArgs(ipc_params, args);
            return false;
          }
          args[i] = data.release();
          break;
        }
        case UINT32_TYPE: {
          uint32 data;
          if (!params->GetParameter32(i, &data)) {
            ReleaseArgs(ipc_params, args);
            return false;
          }
          IPCInt ipc_int(data);
          args[i] = ipc_int.AsVoidPtr();
          break;
        }
        case VOIDPTR_TYPE : {
          void* data;
          if (!params->GetParameterVoidPtr(i, &data)) {
            ReleaseArgs(ipc_params, args);
            return false;
          }
          args[i] = data;
          break;
        }
        case INOUTPTR_TYPE: {
          if (!args[i]) {
            ReleaseArgs(ipc_params, args);
            return false;
          }
          CountedBuffer* buffer = new CountedBuffer(args[i] , size);
          args[i] = buffer;
          break;
        }
        default: break;
      }
    }
  }
  return true;
}

bool SharedMemIPCServer::InvokeCallback(const ServerControl* service_context,
                                        void* ipc_buffer,
                                        CrossCallReturn* call_result) {
  
  SetCallError(SBOX_ERROR_INVALID_IPC, call_result);
  uint32 output_size = 0;
  
  
  
  scoped_ptr<CrossCallParamsEx> params(
      CrossCallParamsEx::CreateFromBuffer(ipc_buffer,
                                          service_context->channel_size,
                                          &output_size));
  if (!params.get())
    return false;

  uint32 tag = params->GetTag();
  COMPILE_ASSERT(0 == INVALID_TYPE, Incorrect_type_enum);
  IPCParams ipc_params = {0};
  ipc_params.ipc_tag = tag;

  void* args[kMaxIpcParams];
  if (!GetArgs(params.get(), &ipc_params, args))
    return false;

  IPCInfo ipc_info = {0};
  ipc_info.ipc_tag = tag;
  ipc_info.client_info = &service_context->target_info;
  Dispatcher* dispatcher = service_context->dispatcher;
  DCHECK(dispatcher);
  bool error = true;
  Dispatcher* handler = NULL;

  Dispatcher::CallbackGeneric callback_generic;
  handler = dispatcher->OnMessageReady(&ipc_params, &callback_generic);
  if (handler) {
    switch (params->GetParamsCount()) {
      case 0: {
        
        Dispatcher::Callback0 callback =
            reinterpret_cast<Dispatcher::Callback0>(callback_generic);
        if (!(handler->*callback)(&ipc_info))
          break;
        error = false;
        break;
      }
      case 1: {
        Dispatcher::Callback1 callback =
            reinterpret_cast<Dispatcher::Callback1>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0]))
          break;
        error = false;
        break;
      }
      case 2: {
        Dispatcher::Callback2 callback =
            reinterpret_cast<Dispatcher::Callback2>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1]))
          break;
        error = false;
        break;
      }
      case 3: {
        Dispatcher::Callback3 callback =
            reinterpret_cast<Dispatcher::Callback3>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2]))
          break;
        error = false;
        break;
      }
      case 4: {
        Dispatcher::Callback4 callback =
            reinterpret_cast<Dispatcher::Callback4>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2],
                                  args[3]))
          break;
        error = false;
        break;
      }
      case 5: {
        Dispatcher::Callback5 callback =
            reinterpret_cast<Dispatcher::Callback5>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4]))
          break;
        error = false;
        break;
      }
      case 6: {
        Dispatcher::Callback6 callback =
            reinterpret_cast<Dispatcher::Callback6>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5]))
          break;
        error = false;
        break;
      }
      case 7: {
        Dispatcher::Callback7 callback =
            reinterpret_cast<Dispatcher::Callback7>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5], args[6]))
          break;
        error = false;
        break;
      }
      case 8: {
        Dispatcher::Callback8 callback =
            reinterpret_cast<Dispatcher::Callback8>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5], args[6], args[7]))
          break;
        error = false;
        break;
      }
      case 9: {
        Dispatcher::Callback9 callback =
            reinterpret_cast<Dispatcher::Callback9>(callback_generic);
        if (!(handler->*callback)(&ipc_info, args[0], args[1], args[2], args[3],
                                  args[4], args[5], args[6], args[7], args[8]))
          break;
        error = false;
        break;
      }
      default:  {
        NOTREACHED();
        break;
      }
    }
  }

  if (error) {
    if (handler)
      SetCallError(SBOX_ERROR_FAILED_IPC, call_result);
  } else {
    memcpy(call_result, &ipc_info.return_info, sizeof(*call_result));
    SetCallSuccess(call_result);
    if (params->IsInOut()) {
      
      
      memcpy(ipc_buffer, params.get(), output_size);
    }
  }

  ReleaseArgs(&ipc_params, args);

  return !error;
}




void __stdcall SharedMemIPCServer::ThreadPingEventReady(void* context,
                                                        unsigned char) {
  if (NULL == context) {
    DCHECK(false);
    return;
  }
  ServerControl* service_context = reinterpret_cast<ServerControl*>(context);
  
  
  LONG last_state =
    ::InterlockedCompareExchange(&service_context->channel->state,
                                 kAckChannel, kBusyChannel);
  if (kBusyChannel != last_state) {
    DCHECK(false);
    return;
  }

  
  
  CrossCallReturn call_result = {0};
  void* buffer = service_context->channel_buffer;

  InvokeCallback(service_context, buffer, &call_result);

  
  
  CrossCallParams* call_params = reinterpret_cast<CrossCallParams*>(buffer);
  memcpy(call_params->GetCallReturn(), &call_result, sizeof(call_result));
  ::InterlockedExchange(&service_context->channel->state, kAckChannel);
  ::SetEvent(service_context->pong_event);
}

bool SharedMemIPCServer::MakeEvents(HANDLE* server_ping, HANDLE* server_pong,
                                    HANDLE* client_ping, HANDLE* client_pong) {
  
  
  const DWORD kDesiredAccess = SYNCHRONIZE | EVENT_MODIFY_STATE;

  
  *server_ping = ::CreateEventW(NULL, FALSE, FALSE, NULL);
  if (!::DuplicateHandle(::GetCurrentProcess(), *server_ping, target_process_,
                         client_ping, kDesiredAccess, FALSE, 0)) {
    return false;
  }
  *server_pong = ::CreateEventW(NULL, FALSE, FALSE, NULL);
  if (!::DuplicateHandle(::GetCurrentProcess(), *server_pong, target_process_,
                         client_pong, kDesiredAccess, FALSE, 0)) {
    return false;
  }
  return true;
}

}  
