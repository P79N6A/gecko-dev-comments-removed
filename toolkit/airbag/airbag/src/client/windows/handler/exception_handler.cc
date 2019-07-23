




























#include <ObjBase.h>

#include <cstdio>

#include "common/windows/string_utils-inl.h"

#include "client/windows/handler/exception_handler.h"
#include "common/windows/guid_string.h"
#include "google_airbag/common/minidump_format.h"

namespace google_airbag {

ExceptionHandler *ExceptionHandler::current_handler_ = NULL;
HANDLE ExceptionHandler::handler_thread_ = NULL;
CRITICAL_SECTION ExceptionHandler::handler_critical_section_;
HANDLE ExceptionHandler::handler_start_semaphore_ = NULL;
HANDLE ExceptionHandler::handler_finish_semaphore_ = NULL;
ExceptionHandler *ExceptionHandler::requesting_handler_ = NULL;
DWORD ExceptionHandler::requesting_thread_id_ = 0;
EXCEPTION_POINTERS *ExceptionHandler::exception_info_ = NULL;
bool ExceptionHandler::handler_return_value_ = false;

ExceptionHandler::ExceptionHandler(const wstring &dump_path,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   bool install_handler)
    : callback_(callback), callback_context_(callback_context),
      dump_path_(dump_path), dbghelp_module_(NULL),
      minidump_write_dump_(NULL), previous_handler_(current_handler_),
      previous_filter_(NULL) {
  if (!handler_thread_) {
    
    
    InitializeCriticalSection(&handler_critical_section_);
    handler_start_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);
    handler_finish_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);

    DWORD thread_id;
    handler_thread_ = CreateThread(NULL,         
                                   64 * 1024,    
                                   ExceptionHandlerThreadMain,
                                   NULL,         
                                   0,            
                                   &thread_id);
  }

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

  if (previous_handler_ == NULL) {
    
    
    TerminateThread(handler_thread_, 1);
    handler_thread_ = NULL;
    DeleteCriticalSection(&handler_critical_section_);
    CloseHandle(handler_start_semaphore_);
    handler_start_semaphore_ = NULL;
    CloseHandle(handler_finish_semaphore_);
    handler_finish_semaphore_ = NULL;
  }
}


DWORD ExceptionHandler::ExceptionHandlerThreadMain(void *lpParameter) {
  while (true) {
    if (WaitForSingleObject(handler_start_semaphore_, INFINITE) ==
        WAIT_OBJECT_0) {
      
      handler_return_value_ = requesting_handler_->WriteMinidumpWithException(
          requesting_thread_id_, exception_info_);

      
      ReleaseSemaphore(handler_finish_semaphore_, 1, NULL);
    }
  }

  
  
  return 0;
}


LONG ExceptionHandler::HandleException(EXCEPTION_POINTERS *exinfo) {
  return current_handler_->WriteMinidumpOnHandlerThread(exinfo) ?
      EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

bool ExceptionHandler::WriteMinidumpOnHandlerThread(EXCEPTION_POINTERS *exinfo) {
  EnterCriticalSection(&handler_critical_section_);

  
  requesting_handler_ = this;
  requesting_thread_id_ = GetCurrentThreadId();
  exception_info_ = exinfo;

  
  ReleaseSemaphore(handler_start_semaphore_, 1, NULL);

  
  WaitForSingleObject(handler_finish_semaphore_, INFINITE);
  bool status = handler_return_value_;

  
  requesting_handler_ = NULL;
  requesting_thread_id_ = 0;
  exception_info_ = NULL;

  LeaveCriticalSection(&handler_critical_section_);

  return status;
}

bool ExceptionHandler::WriteMinidump() {
  bool success = WriteMinidumpOnHandlerThread(NULL);
  UpdateNextID();
  return success;
}


bool ExceptionHandler::WriteMinidump(const wstring &dump_path,
                                     MinidumpCallback callback,
                                     void *callback_context) {
  ExceptionHandler handler(dump_path, callback, callback_context, false);
  return handler.WriteMinidump();
}

bool ExceptionHandler::WriteMinidumpWithException(DWORD requesting_thread_id,
                                                  EXCEPTION_POINTERS *exinfo) {
  wchar_t dump_file_name[MAX_PATH];
  WindowsStringUtils::safe_swprintf(dump_file_name, MAX_PATH, L"%s\\%s.dmp",
                                    dump_path_.c_str(),
                                    next_minidump_id_.c_str());

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
      except_info.ThreadId = requesting_thread_id;
      except_info.ExceptionPointers = exinfo;
      except_info.ClientPointers = FALSE;

      
      
      
      
      
      
      MDRawAirbagInfo airbag_info;
      airbag_info.validity = MD_AIRBAG_INFO_VALID_DUMP_THREAD_ID |
                             MD_AIRBAG_INFO_VALID_REQUESTING_THREAD_ID;
      airbag_info.dump_thread_id = GetCurrentThreadId();
      airbag_info.requesting_thread_id = requesting_thread_id;

      MINIDUMP_USER_STREAM airbag_info_stream;
      airbag_info_stream.Type = MD_AIRBAG_INFO_STREAM;
      airbag_info_stream.BufferSize = sizeof(airbag_info);
      airbag_info_stream.Buffer = &airbag_info;

      MINIDUMP_USER_STREAM_INFORMATION user_streams;
      user_streams.UserStreamCount = 1;
      user_streams.UserStreamArray = &airbag_info_stream;

      
      success = (minidump_write_dump_(GetCurrentProcess(),
                                      GetCurrentProcessId(),
                                      dump_file,
                                      MiniDumpNormal,
                                      exinfo ? &except_info : NULL,
                                      &user_streams,
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
