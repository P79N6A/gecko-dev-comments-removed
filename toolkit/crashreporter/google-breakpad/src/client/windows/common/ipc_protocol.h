




























#ifndef CLIENT_WINDOWS_COMMON_IPC_PROTOCOL_H__
#define CLIENT_WINDOWS_COMMON_IPC_PROTOCOL_H__

#include <Windows.h>
#include <DbgHelp.h>
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {





enum MessageTag {
  MESSAGE_TAG_NONE = 0,
  MESSAGE_TAG_REGISTRATION_REQUEST = 1,
  MESSAGE_TAG_REGISTRATION_RESPONSE = 2,
  MESSAGE_TAG_REGISTRATION_ACK = 3
};


struct ProtocolMessage {
  ProtocolMessage()
      : tag(MESSAGE_TAG_NONE),
        pid(0),
        dump_type(MiniDumpNormal),
        thread_id(0),
        exception_pointers(NULL),
        assert_info(NULL),
        dump_request_handle(NULL),
        dump_generated_handle(NULL),
        server_alive_handle(NULL) {
  }

  
  ProtocolMessage(MessageTag arg_tag,
                  DWORD arg_pid,
                  MINIDUMP_TYPE arg_dump_type,
                  DWORD* arg_thread_id,
                  EXCEPTION_POINTERS** arg_exception_pointers,
                  MDRawAssertionInfo* arg_assert_info,
                  HANDLE arg_dump_request_handle,
                  HANDLE arg_dump_generated_handle,
                  HANDLE arg_server_alive)
    : tag(arg_tag),
      pid(arg_pid),
      dump_type(arg_dump_type),
      thread_id(arg_thread_id),
      exception_pointers(arg_exception_pointers),
      assert_info(arg_assert_info),
      dump_request_handle(arg_dump_request_handle),
      dump_generated_handle(arg_dump_generated_handle),
      server_alive_handle(arg_server_alive) {
  }

  
  MessageTag tag;

  
  DWORD pid;

  
  MINIDUMP_TYPE dump_type;

  
  DWORD* thread_id;

  
  EXCEPTION_POINTERS** exception_pointers;

  
  
  MDRawAssertionInfo* assert_info;

  
  HANDLE dump_request_handle;

  
  HANDLE dump_generated_handle;

  
  
  HANDLE server_alive_handle;

 private:
  
  ProtocolMessage(const ProtocolMessage& msg);
  ProtocolMessage& operator=(const ProtocolMessage& msg);
};

}  

#endif  
