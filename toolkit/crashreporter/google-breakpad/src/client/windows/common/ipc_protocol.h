




























#ifndef CLIENT_WINDOWS_COMMON_IPC_PROTOCOL_H__
#define CLIENT_WINDOWS_COMMON_IPC_PROTOCOL_H__

#include <windows.h>
#include <dbghelp.h>
#include <string>
#include <utility>
#include "common/windows/string_utils-inl.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {


struct CustomInfoEntry {
  
  static const int kNameMaxLength = 64;
  static const int kValueMaxLength = 64;

  CustomInfoEntry() {
    
    set_name(NULL);
    set_value(NULL);
  }

  CustomInfoEntry(const wchar_t* name_arg, const wchar_t* value_arg) {
    set_name(name_arg);
    set_value(value_arg);
  }

  void set_name(const wchar_t* name_arg) {
    if (!name_arg) {
      name[0] = L'\0';
      return;
    }
    WindowsStringUtils::safe_wcscpy(name, kNameMaxLength, name_arg);
  }

  void set_value(const wchar_t* value_arg) {
    if (!value_arg) {
      value[0] = L'\0';
      return;
    }

    WindowsStringUtils::safe_wcscpy(value, kValueMaxLength, value_arg);
  }

  void set(const wchar_t* name_arg, const wchar_t* value_arg) {
    set_name(name_arg);
    set_value(value_arg);
  }

  wchar_t name[kNameMaxLength];
  wchar_t value[kValueMaxLength];
};





enum MessageTag {
  MESSAGE_TAG_NONE = 0,
  MESSAGE_TAG_REGISTRATION_REQUEST = 1,
  MESSAGE_TAG_REGISTRATION_RESPONSE = 2,
  MESSAGE_TAG_REGISTRATION_ACK = 3,
  MESSAGE_TAG_UPLOAD_REQUEST = 4
};

struct CustomClientInfo {
  const CustomInfoEntry* entries;
  size_t count;
};


struct ProtocolMessage {
  ProtocolMessage()
      : tag(MESSAGE_TAG_NONE),
        id(0),
        dump_type(MiniDumpNormal),
        thread_id(0),
        exception_pointers(NULL),
        assert_info(NULL),
        custom_client_info(),
        dump_request_handle(NULL),
        dump_generated_handle(NULL),
        server_alive_handle(NULL) {
  }

  
  ProtocolMessage(MessageTag arg_tag,
                  DWORD arg_id,
                  MINIDUMP_TYPE arg_dump_type,
                  DWORD* arg_thread_id,
                  EXCEPTION_POINTERS** arg_exception_pointers,
                  MDRawAssertionInfo* arg_assert_info,
                  const CustomClientInfo& custom_info,
                  HANDLE arg_dump_request_handle,
                  HANDLE arg_dump_generated_handle,
                  HANDLE arg_server_alive)
    : tag(arg_tag),
      id(arg_id),
      dump_type(arg_dump_type),
      thread_id(arg_thread_id),
      exception_pointers(arg_exception_pointers),
      assert_info(arg_assert_info),
      custom_client_info(custom_info),
      dump_request_handle(arg_dump_request_handle),
      dump_generated_handle(arg_dump_generated_handle),
      server_alive_handle(arg_server_alive) {
  }

  
  MessageTag tag;

  
  
  DWORD id;

  
  MINIDUMP_TYPE dump_type;

  
  DWORD* thread_id;

  
  EXCEPTION_POINTERS** exception_pointers;

  
  
  MDRawAssertionInfo* assert_info;

  
  CustomClientInfo custom_client_info;

  
  HANDLE dump_request_handle;

  
  HANDLE dump_generated_handle;

  
  
  HANDLE server_alive_handle;

 private:
  
  ProtocolMessage(const ProtocolMessage& msg);
  ProtocolMessage& operator=(const ProtocolMessage& msg);
};

}  

#endif  
