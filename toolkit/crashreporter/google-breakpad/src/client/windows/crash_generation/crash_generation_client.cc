




























#include "client/windows/crash_generation/crash_generation_client.h"
#include <cassert>
#include <utility>
#include "client/windows/common/ipc_protocol.h"

namespace google_breakpad {

const int kPipeBusyWaitTimeoutMs = 2000;

#ifdef _DEBUG
const DWORD kWaitForServerTimeoutMs = INFINITE;
#else
const DWORD kWaitForServerTimeoutMs = 15000;
#endif

const int kPipeConnectMaxAttempts = 2;

const DWORD kPipeDesiredAccess = FILE_READ_DATA |
                                 FILE_WRITE_DATA |
                                 FILE_WRITE_ATTRIBUTES;

const DWORD kPipeFlagsAndAttributes = SECURITY_IDENTIFICATION |
                                      SECURITY_SQOS_PRESENT;

const DWORD kPipeMode = PIPE_READMODE_MESSAGE;

const size_t kWaitEventCount = 2;




































CrashGenerationClient::CrashGenerationClient(
    const wchar_t* pipe_name,
    MINIDUMP_TYPE dump_type,
    const CustomClientInfo* custom_info)
        : pipe_name_(pipe_name),
          dump_type_(dump_type),
          thread_id_(0),
          server_process_id_(0),
          crash_event_(NULL),
          crash_generated_(NULL),
          server_alive_(NULL),
          exception_pointers_(NULL),
          custom_info_() {
  memset(&assert_info_, 0, sizeof(assert_info_));
  if (custom_info) {
    custom_info_ = *custom_info;
  }
}

CrashGenerationClient::~CrashGenerationClient() {
  if (crash_event_) {
    CloseHandle(crash_event_);
  }

  if (crash_generated_) {
    CloseHandle(crash_generated_);
  }

  if (server_alive_) {
    CloseHandle(server_alive_);
  }
}


































bool CrashGenerationClient::Register() {
  HANDLE pipe = ConnectToServer();
  if (!pipe) {
    return false;
  }

  bool success = RegisterClient(pipe);
  CloseHandle(pipe);
  return success;
}

HANDLE CrashGenerationClient::ConnectToServer() {
  HANDLE pipe = ConnectToPipe(pipe_name_.c_str(),
                              kPipeDesiredAccess,
                              kPipeFlagsAndAttributes);
  if (!pipe) {
    return NULL;
  }

  DWORD mode = kPipeMode;
  if (!SetNamedPipeHandleState(pipe, &mode, NULL, NULL)) {
    CloseHandle(pipe);
    pipe = NULL;
  }

  return pipe;
}

bool CrashGenerationClient::RegisterClient(HANDLE pipe) {
  ProtocolMessage msg(MESSAGE_TAG_REGISTRATION_REQUEST,
                      GetCurrentProcessId(),
                      dump_type_,
                      &thread_id_,
                      &exception_pointers_,
                      &assert_info_,
                      custom_info_,
                      NULL,
                      NULL,
                      NULL);
  ProtocolMessage reply;
  DWORD bytes_count = 0;
  
  
  
  if (!TransactNamedPipe(pipe,
                         &msg,
                         sizeof(msg),
                         &reply,
                         sizeof(ProtocolMessage),
                         &bytes_count,
                         NULL)) {
    return false;
  }

  if (!ValidateResponse(reply)) {
    return false;
  }

  ProtocolMessage ack_msg;
  ack_msg.tag = MESSAGE_TAG_REGISTRATION_ACK;

  if (!WriteFile(pipe, &ack_msg, sizeof(ack_msg), &bytes_count, NULL)) {
    return false;
  }
  crash_event_ = reply.dump_request_handle;
  crash_generated_ = reply.dump_generated_handle;
  server_alive_ = reply.server_alive_handle;
  server_process_id_ = reply.pid;

  return true;
}

HANDLE CrashGenerationClient::ConnectToPipe(const wchar_t* pipe_name,
                                            DWORD pipe_access,
                                            DWORD flags_attrs) {
  for (int i = 0; i < kPipeConnectMaxAttempts; ++i) {
    HANDLE pipe = CreateFile(pipe_name,
                             pipe_access,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             flags_attrs,
                             NULL);
    if (pipe != INVALID_HANDLE_VALUE) {
      return pipe;
    }

    
    
    if (GetLastError() != ERROR_PIPE_BUSY) {
      break;
    }

    
    if (!WaitNamedPipe(pipe_name, kPipeBusyWaitTimeoutMs)) {
      break;
    }
  }

  return NULL;
}

bool CrashGenerationClient::ValidateResponse(
    const ProtocolMessage& msg) const {
  return (msg.tag == MESSAGE_TAG_REGISTRATION_RESPONSE) &&
         (msg.pid != 0) &&
         (msg.dump_request_handle != NULL) &&
         (msg.dump_generated_handle != NULL) &&
         (msg.server_alive_handle != NULL);
}

bool CrashGenerationClient::IsRegistered() const {
  return crash_event_ != NULL;
}

bool CrashGenerationClient::RequestDump(EXCEPTION_POINTERS* ex_info) {
  if (!IsRegistered()) {
    return false;
  }

  exception_pointers_ = ex_info;
  thread_id_ = GetCurrentThreadId();

  assert_info_.line = 0;
  assert_info_.type = 0;
  assert_info_.expression[0] = 0;
  assert_info_.file[0] = 0;
  assert_info_.function[0] = 0;

  return SignalCrashEventAndWait();
}

bool CrashGenerationClient::RequestDump(MDRawAssertionInfo* assert_info) {
  if (!IsRegistered()) {
    return false;
  }

  exception_pointers_ = NULL;

  if (assert_info) {
    memcpy(&assert_info_, assert_info, sizeof(assert_info_));
  } else {
    memset(&assert_info_, 0, sizeof(assert_info_));
  }

  thread_id_ = GetCurrentThreadId();

  return SignalCrashEventAndWait();
}

bool CrashGenerationClient::SignalCrashEventAndWait() {
  assert(crash_event_);
  assert(crash_generated_);
  assert(server_alive_);

  
  
  
  if (!ResetEvent(crash_generated_)) {
    return false;
  }

  if (!SetEvent(crash_event_)) {
    return false;
  }

  HANDLE wait_handles[kWaitEventCount] = {crash_generated_, server_alive_};

  DWORD result = WaitForMultipleObjects(kWaitEventCount,
                                        wait_handles,
                                        FALSE,
                                        kWaitForServerTimeoutMs);

  
  
  return result == WAIT_OBJECT_0;
}

}  
