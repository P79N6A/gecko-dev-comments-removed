




























#include "client/windows/crash_generation/crash_generation_server.h"
#include <windows.h>
#include <cassert>
#include <list>
#include "client/windows/common/auto_critical_section.h"
#include "processor/scoped_ptr.h"

namespace google_breakpad {


static const size_t kOutBufferSize = 64;


static const size_t kInBufferSize = 64;


static const DWORD kDumpRequestEventAccess = EVENT_MODIFY_STATE;


static const DWORD kDumpGeneratedEventAccess = EVENT_MODIFY_STATE |
                                               SYNCHRONIZE;


static const DWORD kMutexAccess = SYNCHRONIZE;


static const DWORD kPipeAttr = FILE_FLAG_FIRST_PIPE_INSTANCE |
                               PIPE_ACCESS_DUPLEX |
                               FILE_FLAG_OVERLAPPED;


static const DWORD kPipeMode = PIPE_TYPE_MESSAGE |
                               PIPE_READMODE_MESSAGE |
                               PIPE_WAIT;






static const ULONG kPipeIOThreadFlags = WT_EXECUTEINWAITTHREAD;



static const ULONG kDumpRequestThreadFlags = WT_EXECUTEINWAITTHREAD |
                                             WT_EXECUTELONGFUNCTION;



static const int kShutdownDelayMs = 10000;


static const int kShutdownSleepIntervalMs = 5;

static bool IsClientRequestValid(const ProtocolMessage& msg) {
  return msg.tag == MESSAGE_TAG_REGISTRATION_REQUEST &&
         msg.pid != 0 &&
         msg.thread_id != NULL &&
         msg.exception_pointers != NULL &&
         msg.assert_info != NULL;
}

CrashGenerationServer::CrashGenerationServer(
    const std::wstring& pipe_name,
    SECURITY_ATTRIBUTES* pipe_sec_attrs,
    OnClientConnectedCallback connect_callback,
    void* connect_context,
    OnClientDumpRequestCallback dump_callback,
    void* dump_context,
    OnClientExitedCallback exit_callback,
    void* exit_context,
    bool generate_dumps,
    const std::wstring* dump_path)
    : pipe_name_(pipe_name),
      pipe_sec_attrs_(pipe_sec_attrs),
      pipe_(NULL),
      pipe_wait_handle_(NULL),
      server_alive_handle_(NULL),
      connect_callback_(connect_callback),
      connect_context_(connect_context),
      dump_callback_(dump_callback),
      dump_context_(dump_context),
      exit_callback_(exit_callback),
      exit_context_(exit_context),
      generate_dumps_(generate_dumps),
      dump_generator_(NULL),
      server_state_(IPC_SERVER_STATE_INITIAL),
      shutting_down_(false),
      overlapped_(),
      client_info_(NULL),
      cleanup_item_count_(0) {
  InitializeCriticalSection(&clients_sync_);

  if (dump_path) {
    dump_generator_.reset(new MinidumpGenerator(*dump_path));
  }
}

CrashGenerationServer::~CrashGenerationServer() {
  
  shutting_down_ = true;

  
  
  
  
  
  
  
  DisconnectNamedPipe(pipe_);
  int num_tries = 100;
  while (num_tries-- && server_state_ != IPC_SERVER_STATE_ERROR) {
    Sleep(10);
  }

  
  if (pipe_wait_handle_) {
    
    UnregisterWaitEx(pipe_wait_handle_, INVALID_HANDLE_VALUE);
  }

  
  if (pipe_) {
    CloseHandle(pipe_);
  }

  
  
  {
    AutoCriticalSection lock(&clients_sync_);

    std::list<ClientInfo*>::iterator iter;
    for (iter = clients_.begin(); iter != clients_.end(); ++iter) {
      ClientInfo* client_info = *iter;
      client_info->UnregisterWaits();
    }
  }

  
  
  int total_wait = 0;
  while (cleanup_item_count_ > 0) {
    Sleep(kShutdownSleepIntervalMs);

    total_wait += kShutdownSleepIntervalMs;

    if (total_wait >= kShutdownDelayMs) {
      break;
    }
  }

  
  
  {
    AutoCriticalSection lock(&clients_sync_);

    std::list<ClientInfo*>::iterator iter;
    for (iter = clients_.begin(); iter != clients_.end(); ++iter) {
      ClientInfo* client_info = *iter;
      delete client_info;
    }
  }

  if (server_alive_handle_) {
    
    
    ReleaseMutex(server_alive_handle_);
    CloseHandle(server_alive_handle_);
  }

  DeleteCriticalSection(&clients_sync_);
}

bool CrashGenerationServer::Start() {
  server_state_ = IPC_SERVER_STATE_INITIAL;

  server_alive_handle_ = CreateMutex(NULL, TRUE, NULL);
  if (!server_alive_handle_) {
    return false;
  }

  
  overlapped_.hEvent = CreateEvent(NULL,   
                                   TRUE,   
                                   FALSE,  
                                   NULL);  
  if (!overlapped_.hEvent) {
    return false;
  }

  
  RegisterWaitForSingleObject(&pipe_wait_handle_,
                              overlapped_.hEvent,
                              OnPipeConnected,
                              this,
                              INFINITE,
                              kPipeIOThreadFlags);

  pipe_ = CreateNamedPipe(pipe_name_.c_str(),
                          kPipeAttr,
                          kPipeMode,
                          1,
                          kOutBufferSize,
                          kInBufferSize,
                          0,
                          pipe_sec_attrs_);
  if (pipe_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  
  
  return SetEvent(overlapped_.hEvent) != FALSE;
}






void CrashGenerationServer::HandleErrorState() {
  assert(server_state_ == IPC_SERVER_STATE_ERROR);

  
  
  if (shutting_down_) {
    return;
  }

  if (pipe_wait_handle_) {
    UnregisterWait(pipe_wait_handle_);
    pipe_wait_handle_ = NULL;
  }

  if (pipe_) {
    CloseHandle(pipe_);
    pipe_ = NULL;
  }

  if (overlapped_.hEvent) {
    CloseHandle(overlapped_.hEvent);
    overlapped_.hEvent = NULL;
  }
}






void CrashGenerationServer::HandleInitialState() {
  assert(server_state_ == IPC_SERVER_STATE_INITIAL);

  if (!ResetEvent(overlapped_.hEvent)) {
    server_state_ = IPC_SERVER_STATE_ERROR;
    return;
  }

  bool success = ConnectNamedPipe(pipe_, &overlapped_) != FALSE;

  
  
  
  assert(!success);

  DWORD error_code = GetLastError();
  switch (error_code) {
    case ERROR_IO_PENDING:
      server_state_ = IPC_SERVER_STATE_CONNECTING;
      break;

    case ERROR_PIPE_CONNECTED:
      if (SetEvent(overlapped_.hEvent)) {
        server_state_ = IPC_SERVER_STATE_CONNECTED;
      } else {
        server_state_ = IPC_SERVER_STATE_ERROR;
      }
      break;

    default:
      server_state_ = IPC_SERVER_STATE_ERROR;
      break;
  }
}







void CrashGenerationServer::HandleConnectingState() {
  assert(server_state_ == IPC_SERVER_STATE_CONNECTING);

  DWORD bytes_count = 0;
  bool success = GetOverlappedResult(pipe_,
                                     &overlapped_,
                                     &bytes_count,
                                     FALSE) != FALSE;

  if (success) {
    server_state_ = IPC_SERVER_STATE_CONNECTED;
    return;
  }

  if (GetLastError() != ERROR_IO_INCOMPLETE) {
    server_state_ = IPC_SERVER_STATE_DISCONNECTING;
  }
}





void CrashGenerationServer::HandleConnectedState() {
  assert(server_state_ == IPC_SERVER_STATE_CONNECTED);

  DWORD bytes_count = 0;
  memset(&msg_, 0, sizeof(msg_));
  bool success = ReadFile(pipe_,
                          &msg_,
                          sizeof(msg_),
                          &bytes_count,
                          &overlapped_) != FALSE;

  
  
  
  
  
  if (success || GetLastError() == ERROR_IO_PENDING) {
    server_state_ = IPC_SERVER_STATE_READING;
  } else {
    server_state_ = IPC_SERVER_STATE_DISCONNECTING;
  }
}





void CrashGenerationServer::HandleReadingState() {
  assert(server_state_ == IPC_SERVER_STATE_READING);

  DWORD bytes_count = 0;
  bool success = GetOverlappedResult(pipe_,
                                     &overlapped_,
                                     &bytes_count,
                                     FALSE) != FALSE;

  if (success && bytes_count == sizeof(ProtocolMessage)) {
    server_state_ = IPC_SERVER_STATE_READ_DONE;
    return;
  }

  DWORD error_code;
  error_code = GetLastError();

  
  
  
  assert(error_code != ERROR_IO_INCOMPLETE);

  server_state_ = IPC_SERVER_STATE_DISCONNECTING;
}







void CrashGenerationServer::HandleReadDoneState() {
  assert(server_state_ == IPC_SERVER_STATE_READ_DONE);

  if (!IsClientRequestValid(msg_)) {
    server_state_ = IPC_SERVER_STATE_DISCONNECTING;
    return;
  }

  scoped_ptr<ClientInfo> client_info(
      new ClientInfo(this,
                     msg_.pid,
                     msg_.dump_type,
                     msg_.thread_id,
                     msg_.exception_pointers,
                     msg_.assert_info,
                     msg_.custom_client_info));

  if (!client_info->Initialize()) {
    server_state_ = IPC_SERVER_STATE_DISCONNECTING;
    return;
  }

  if (!RespondToClient(client_info.get())) {
    server_state_ = IPC_SERVER_STATE_DISCONNECTING;
    return;
  }

  
  
  
  
  
  server_state_ = IPC_SERVER_STATE_WRITING;
  client_info_ = client_info.release();
}





void CrashGenerationServer::HandleWritingState() {
  assert(server_state_ == IPC_SERVER_STATE_WRITING);

  DWORD bytes_count = 0;
  bool success = GetOverlappedResult(pipe_,
                                     &overlapped_,
                                     &bytes_count,
                                     FALSE) != FALSE;

  if (success) {
    server_state_ = IPC_SERVER_STATE_WRITE_DONE;
    return;
  }

  DWORD error_code;
  error_code = GetLastError();

  
  
  
  assert(error_code != ERROR_IO_INCOMPLETE);

  server_state_ = IPC_SERVER_STATE_DISCONNECTING;
}





void CrashGenerationServer::HandleWriteDoneState() {
  assert(server_state_ == IPC_SERVER_STATE_WRITE_DONE);

  server_state_ = IPC_SERVER_STATE_READING_ACK;

  DWORD bytes_count = 0;
  bool success = ReadFile(pipe_,
                          &msg_,
                          sizeof(msg_),
                          &bytes_count,
                          &overlapped_) != FALSE;

  if (success) {
    return;
  }

  DWORD error_code = GetLastError();

  if (error_code != ERROR_IO_PENDING) {
    server_state_ = IPC_SERVER_STATE_DISCONNECTING;
  }
}



void CrashGenerationServer::HandleReadingAckState() {
  assert(server_state_ == IPC_SERVER_STATE_READING_ACK);

  DWORD bytes_count = 0;
  bool success = GetOverlappedResult(pipe_,
                                     &overlapped_,
                                     &bytes_count,
                                     FALSE) != FALSE;

  if (success) {
    
    
    if (connect_callback_) {
      connect_callback_(connect_context_, client_info_);
    }
  } else {
    DWORD error_code = GetLastError();

    
    
    
    assert(error_code != ERROR_IO_INCOMPLETE);
  }

  server_state_ = IPC_SERVER_STATE_DISCONNECTING;
}





void CrashGenerationServer::HandleDisconnectingState() {
  assert(server_state_ == IPC_SERVER_STATE_DISCONNECTING);

  
  client_info_ = NULL;

  overlapped_.Internal = NULL;
  overlapped_.InternalHigh = NULL;
  overlapped_.Offset = 0;
  overlapped_.OffsetHigh = 0;
  overlapped_.Pointer = NULL;

  if (!ResetEvent(overlapped_.hEvent)) {
    server_state_ = IPC_SERVER_STATE_ERROR;
    return;
  }

  if (!DisconnectNamedPipe(pipe_)) {
    server_state_ = IPC_SERVER_STATE_ERROR;
    return;
  }

  
  
  if (shutting_down_) {
    return;
  }

  server_state_ = IPC_SERVER_STATE_INITIAL;
  if (!SetEvent(overlapped_.hEvent)) {
    server_state_ = IPC_SERVER_STATE_ERROR;
  }
}

bool CrashGenerationServer::PrepareReply(const ClientInfo& client_info,
                                         ProtocolMessage* reply) const {
  reply->tag = MESSAGE_TAG_REGISTRATION_RESPONSE;
  reply->pid = GetCurrentProcessId();

  if (CreateClientHandles(client_info, reply)) {
    return true;
  }

  if (reply->dump_request_handle) {
    CloseHandle(reply->dump_request_handle);
  }

  if (reply->dump_generated_handle) {
    CloseHandle(reply->dump_generated_handle);
  }

  if (reply->server_alive_handle) {
    CloseHandle(reply->server_alive_handle);
  }

  return false;
}

bool CrashGenerationServer::CreateClientHandles(const ClientInfo& client_info,
                                                ProtocolMessage* reply) const {
  HANDLE current_process = GetCurrentProcess();
  if (!DuplicateHandle(current_process,
                       client_info.dump_requested_handle(),
                       client_info.process_handle(),
                       &reply->dump_request_handle,
                       kDumpRequestEventAccess,
                       FALSE,
                       0)) {
    return false;
  }

  if (!DuplicateHandle(current_process,
                       client_info.dump_generated_handle(),
                       client_info.process_handle(),
                       &reply->dump_generated_handle,
                       kDumpGeneratedEventAccess,
                       FALSE,
                       0)) {
    return false;
  }

  if (!DuplicateHandle(current_process,
                       server_alive_handle_,
                       client_info.process_handle(),
                       &reply->server_alive_handle,
                       kMutexAccess,
                       FALSE,
                       0)) {
    return false;
  }

  return true;
}

bool CrashGenerationServer::RespondToClient(ClientInfo* client_info) {
  ProtocolMessage reply;
  if (!PrepareReply(*client_info, &reply)) {
    return false;
  }

  if (!AddClient(client_info)) {
    return false;
  }

  DWORD bytes_count = 0;
  bool success = WriteFile(pipe_,
                           &reply,
                           sizeof(reply),
                           &bytes_count,
                           &overlapped_) != FALSE;

  return success || GetLastError() == ERROR_IO_PENDING;
}




void CrashGenerationServer::HandleConnectionRequest() {
  
  
  if (shutting_down_) {
    server_state_ = IPC_SERVER_STATE_ERROR;
    ResetEvent(overlapped_.hEvent);
    return;
  }

  switch (server_state_) {
    case IPC_SERVER_STATE_ERROR:
      HandleErrorState();
      break;

    case IPC_SERVER_STATE_INITIAL:
      HandleInitialState();
      break;

    case IPC_SERVER_STATE_CONNECTING:
      HandleConnectingState();
      break;

    case IPC_SERVER_STATE_CONNECTED:
      HandleConnectedState();
      break;

    case IPC_SERVER_STATE_READING:
      HandleReadingState();
      break;

    case IPC_SERVER_STATE_READ_DONE:
      HandleReadDoneState();
      break;

    case IPC_SERVER_STATE_WRITING:
      HandleWritingState();
      break;

    case IPC_SERVER_STATE_WRITE_DONE:
      HandleWriteDoneState();
      break;

    case IPC_SERVER_STATE_READING_ACK:
      HandleReadingAckState();
      break;

    case IPC_SERVER_STATE_DISCONNECTING:
      HandleDisconnectingState();
      break;

    default:
      assert(false);
      
      
      server_state_ = IPC_SERVER_STATE_ERROR;
      break;
  }
}

bool CrashGenerationServer::AddClient(ClientInfo* client_info) {
  HANDLE request_wait_handle = NULL;
  if (!RegisterWaitForSingleObject(&request_wait_handle,
                                   client_info->dump_requested_handle(),
                                   OnDumpRequest,
                                   client_info,
                                   INFINITE,
                                   kDumpRequestThreadFlags)) {
    return false;
  }

  client_info->set_dump_request_wait_handle(request_wait_handle);

  
  HANDLE process_wait_handle = NULL;
  if (!RegisterWaitForSingleObject(&process_wait_handle,
                                   client_info->process_handle(),
                                   OnClientEnd,
                                   client_info,
                                   INFINITE,
                                   WT_EXECUTEONLYONCE)) {
    return false;
  }

  client_info->set_process_exit_wait_handle(process_wait_handle);

  
  {
    AutoCriticalSection lock(&clients_sync_);
    clients_.push_back(client_info);
  }

  return true;
}


void CALLBACK CrashGenerationServer::OnPipeConnected(void* context, BOOLEAN) {
  assert (context);

  CrashGenerationServer* obj =
      reinterpret_cast<CrashGenerationServer*>(context);
  obj->HandleConnectionRequest();
}


void CALLBACK CrashGenerationServer::OnDumpRequest(void* context, BOOLEAN) {
  assert(context);
  ClientInfo* client_info = reinterpret_cast<ClientInfo*>(context);
  client_info->PopulateCustomInfo();

  CrashGenerationServer* crash_server = client_info->crash_server();
  assert(crash_server);
  crash_server->HandleDumpRequest(*client_info);

  ResetEvent(client_info->dump_requested_handle());
}


void CALLBACK CrashGenerationServer::OnClientEnd(void* context, BOOLEAN) {
  assert(context);
  ClientInfo* client_info = reinterpret_cast<ClientInfo*>(context);

  CrashGenerationServer* crash_server = client_info->crash_server();
  assert(crash_server);

  InterlockedIncrement(&crash_server->cleanup_item_count_);

  if (!QueueUserWorkItem(CleanupClient, context, WT_EXECUTEDEFAULT)) {
    InterlockedDecrement(&crash_server->cleanup_item_count_);
  }
}


DWORD WINAPI CrashGenerationServer::CleanupClient(void* context) {
  assert(context);
  ClientInfo* client_info = reinterpret_cast<ClientInfo*>(context);

  CrashGenerationServer* crash_server = client_info->crash_server();
  assert(crash_server);

  if (crash_server->exit_callback_) {
    crash_server->exit_callback_(crash_server->exit_context_, client_info);
  }

  crash_server->DoCleanup(client_info);

  InterlockedDecrement(&crash_server->cleanup_item_count_);
  return 0;
}

void CrashGenerationServer::DoCleanup(ClientInfo* client_info) {
  assert(client_info);

  
  {
    AutoCriticalSection lock(&clients_sync_);
    clients_.remove(client_info);
  }

  delete client_info;
}

void CrashGenerationServer::HandleDumpRequest(const ClientInfo& client_info) {
  
  
  
  std::wstring dump_path;
  if (generate_dumps_) {
    if (!GenerateDump(client_info, &dump_path)) {
      return;
    }
  }

  if (dump_callback_) {
    std::wstring* ptr_dump_path = (dump_path == L"") ? NULL : &dump_path;
    dump_callback_(dump_context_, &client_info, ptr_dump_path);
  }

  SetEvent(client_info.dump_generated_handle());
}

bool CrashGenerationServer::GenerateDump(const ClientInfo& client,
                                         std::wstring* dump_path) {
  assert(client.pid() != 0);
  assert(client.process_handle());

  
  
  EXCEPTION_POINTERS* client_ex_info = NULL;
  if (!client.GetClientExceptionInfo(&client_ex_info)) {
    return false;
  }

  DWORD client_thread_id = 0;
  if (!client.GetClientThreadId(&client_thread_id)) {
    return false;
  }

  return dump_generator_->WriteMinidump(client.process_handle(),
                                        client.pid(),
                                        client_thread_id,
                                        GetCurrentThreadId(),
                                        client_ex_info,
                                        client.assert_info(),
                                        client.dump_type(),
                                        true,
                                        dump_path);
}

}  
