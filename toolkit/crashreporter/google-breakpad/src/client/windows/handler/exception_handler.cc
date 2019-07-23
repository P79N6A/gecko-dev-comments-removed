




























#include <ObjBase.h>

#include <cstdio>

#include "client/windows/handler/exception_handler.h"
#include "common/windows/guid_string.h"

namespace google_airbag {

ExceptionHandler *ExceptionHandler::current_handler_ = NULL;

ExceptionHandler::ExceptionHandler(const wstring &dump_path,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   bool install_handler)
    : callback_(callback), callback_context_(callback_context),
      dump_path_(dump_path), dbghelp_module_(NULL),
      minidump_write_dump_(NULL), previous_handler_(current_handler_),
      previous_filter_(NULL) {
  UpdateNextID();
  dbghelp_module_ = LoadLibrary(L"dbghelp.dll");
  if (dbghelp_module_) {
    minidump_write_dump_ = reinterpret_cast<MiniDumpWriteDump_type>(
        GetProcAddress(dbghelp_module_, "MiniDumpWriteDump"));
  }
  if (install_handler) {
    previous_filter_ = SetUnhandledExceptionFilter(HandleException);
    current_handler_ = this;
  }
}

ExceptionHandler::~ExceptionHandler() {
  if (dbghelp_module_) {
    FreeLibrary(dbghelp_module_);
  }
  if (current_handler_ == this) {
    SetUnhandledExceptionFilter(previous_filter_);
    current_handler_ = previous_handler_;
  }
}


LONG ExceptionHandler::HandleException(EXCEPTION_POINTERS *exinfo) {
  if (!current_handler_->WriteMinidumpWithException(exinfo)) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

bool ExceptionHandler::WriteMinidump() {
  bool success = WriteMinidumpWithException(NULL);
  UpdateNextID();
  return success;
}


bool ExceptionHandler::WriteMinidump(const wstring &dump_path,
                                     MinidumpCallback callback,
                                     void *callback_context) {
  ExceptionHandler handler(dump_path, callback, callback_context, false);
  return handler.WriteMinidump();
}

bool ExceptionHandler::WriteMinidumpWithException(EXCEPTION_POINTERS *exinfo) {
  wchar_t dump_file_name[MAX_PATH];
  swprintf_s(dump_file_name, MAX_PATH, L"%s\\%s.dmp",
             dump_path_.c_str(), next_minidump_id_.c_str());

  bool success = false;
  if (minidump_write_dump_) {
    HANDLE dump_file = CreateFile(dump_file_name,
                                  GENERIC_WRITE,
                                  FILE_SHARE_WRITE,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    if (dump_file != INVALID_HANDLE_VALUE) {
      MINIDUMP_EXCEPTION_INFORMATION except_info;
      except_info.ThreadId = GetCurrentThreadId();
      except_info.ExceptionPointers = exinfo;
      except_info.ClientPointers = FALSE;

      
      success = (minidump_write_dump_(GetCurrentProcess(),
                                      GetCurrentProcessId(),
                                      dump_file,
                                      MiniDumpNormal,
                                      &except_info,
                                      NULL,
                                      NULL) == TRUE);

      CloseHandle(dump_file);
    }
  }

  if (callback_) {
    callback_(next_minidump_id_, callback_context_, success);
  }
  

  return success;
}

void ExceptionHandler::UpdateNextID() {
  GUID id;
  CoCreateGuid(&id);
  next_minidump_id_ = GUIDString::GUIDToWString(&id);
}

}  
