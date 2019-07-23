




























#include <ObjBase.h>

#include <cassert>
#include <cstdio>

#include "common/windows/string_utils-inl.h"

#include "client/windows/handler/exception_handler.h"
#include "common/windows/guid_string.h"

namespace google_breakpad {

static const int kExceptionHandlerThreadInitialStackSize = 64 * 1024;

vector<ExceptionHandler *> *ExceptionHandler::handler_stack_ = NULL;
LONG ExceptionHandler::handler_stack_index_ = 0;
CRITICAL_SECTION ExceptionHandler::handler_stack_critical_section_;
bool ExceptionHandler::handler_stack_critical_section_initialized_ = false;

ExceptionHandler::ExceptionHandler(const wstring &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   int handler_types)
    : filter_(filter),
      callback_(callback),
      callback_context_(callback_context),
      dump_path_(),
      next_minidump_id_(),
      next_minidump_path_(),
      dump_path_c_(),
      next_minidump_id_c_(NULL),
      next_minidump_path_c_(NULL),
      dbghelp_module_(NULL),
      minidump_write_dump_(NULL),
      handler_types_(handler_types),
      previous_filter_(NULL),
      previous_pch_(NULL),
      handler_thread_(0),
      handler_critical_section_(),
      handler_start_semaphore_(NULL),
      handler_finish_semaphore_(NULL),
      requesting_thread_id_(0),
      exception_info_(NULL),
      assertion_(NULL),
      handler_return_value_(false) {
#if _MSC_VER >= 1400  
  previous_iph_ = NULL;
#endif  

  
  
  set_dump_path(dump_path);

  
  
  
  
  
  InitializeCriticalSection(&handler_critical_section_);
  handler_start_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);
  handler_finish_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);

  DWORD thread_id;
  handler_thread_ = CreateThread(NULL,         
                                 kExceptionHandlerThreadInitialStackSize,
                                 ExceptionHandlerThreadMain,
                                 this,         
                                 0,            
                                 &thread_id);

  dbghelp_module_ = LoadLibrary(L"dbghelp.dll");
  if (dbghelp_module_) {
    minidump_write_dump_ = reinterpret_cast<MiniDumpWriteDump_type>(
        GetProcAddress(dbghelp_module_, "MiniDumpWriteDump"));
  }

  if (handler_types != HANDLER_NONE) {
    if (!handler_stack_critical_section_initialized_) {
      InitializeCriticalSection(&handler_stack_critical_section_);
      handler_stack_critical_section_initialized_ = true;
    }

    EnterCriticalSection(&handler_stack_critical_section_);

    
    
    if (!handler_stack_) {
      handler_stack_ = new vector<ExceptionHandler *>();
    }
    handler_stack_->push_back(this);

    if (handler_types & HANDLER_EXCEPTION)
      previous_filter_ = SetUnhandledExceptionFilter(HandleException);

#if _MSC_VER >= 1400  
    if (handler_types & HANDLER_INVALID_PARAMETER)
      previous_iph_ = _set_invalid_parameter_handler(HandleInvalidParameter);
#endif  

    if (handler_types & HANDLER_PURECALL)
      previous_pch_ = _set_purecall_handler(HandlePureVirtualCall);

    LeaveCriticalSection(&handler_stack_critical_section_);
  }
}

ExceptionHandler::~ExceptionHandler() {
  if (dbghelp_module_) {
    FreeLibrary(dbghelp_module_);
  }

  if (handler_types_ != HANDLER_NONE) {
    EnterCriticalSection(&handler_stack_critical_section_);

    if (handler_types_ & HANDLER_EXCEPTION)
      SetUnhandledExceptionFilter(previous_filter_);

#if _MSC_VER >= 1400  
    if (handler_types_ & HANDLER_INVALID_PARAMETER)
      _set_invalid_parameter_handler(previous_iph_);
#endif  

    if (handler_types_ & HANDLER_PURECALL)
      _set_purecall_handler(previous_pch_);

    if (handler_stack_->back() == this) {
      handler_stack_->pop_back();
    } else {
      
      
      fprintf(stderr, "warning: removing Breakpad handler out of order\n");
      for (vector<ExceptionHandler *>::iterator iterator =
               handler_stack_->begin();
           iterator != handler_stack_->end();
           ++iterator) {
        if (*iterator == this) {
          handler_stack_->erase(iterator);
        }
      }
    }

    if (handler_stack_->empty()) {
      
      
      delete handler_stack_;
      handler_stack_ = NULL;
    }

    LeaveCriticalSection(&handler_stack_critical_section_);
  }

  
  TerminateThread(handler_thread_, 1);
  DeleteCriticalSection(&handler_critical_section_);
  CloseHandle(handler_start_semaphore_);
  CloseHandle(handler_finish_semaphore_);
}


DWORD ExceptionHandler::ExceptionHandlerThreadMain(void *lpParameter) {
  ExceptionHandler *self = reinterpret_cast<ExceptionHandler *>(lpParameter);
  assert(self);

  while (true) {
    if (WaitForSingleObject(self->handler_start_semaphore_, INFINITE) ==
        WAIT_OBJECT_0) {
      
      self->handler_return_value_ = self->WriteMinidumpWithException(
          self->requesting_thread_id_, self->exception_info_, self->assertion_);

      
      ReleaseSemaphore(self->handler_finish_semaphore_, 1, NULL);
    }
  }

  
  
  return 0;
}






class AutoExceptionHandler {
 public:
  AutoExceptionHandler() {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    EnterCriticalSection(&ExceptionHandler::handler_stack_critical_section_);
    handler_ = ExceptionHandler::handler_stack_->at(
        ExceptionHandler::handler_stack_->size() -
        ++ExceptionHandler::handler_stack_index_);
    LeaveCriticalSection(&ExceptionHandler::handler_stack_critical_section_);

    
    
    SetUnhandledExceptionFilter(handler_->previous_filter_);
#if _MSC_VER >= 1400  
    _set_invalid_parameter_handler(handler_->previous_iph_);
#endif  
    _set_purecall_handler(handler_->previous_pch_);
  }

  ~AutoExceptionHandler() {
    
    SetUnhandledExceptionFilter(ExceptionHandler::HandleException);
#if _MSC_VER >= 1400  
    _set_invalid_parameter_handler(ExceptionHandler::HandleInvalidParameter);
#endif  
    _set_purecall_handler(ExceptionHandler::HandlePureVirtualCall);

    EnterCriticalSection(&ExceptionHandler::handler_stack_critical_section_);
    --ExceptionHandler::handler_stack_index_;
    LeaveCriticalSection(&ExceptionHandler::handler_stack_critical_section_);
  }

  ExceptionHandler *get_handler() const { return handler_; }

 private:
  ExceptionHandler *handler_;
};


LONG ExceptionHandler::HandleException(EXCEPTION_POINTERS *exinfo) {
  AutoExceptionHandler auto_exception_handler;
  ExceptionHandler *current_handler = auto_exception_handler.get_handler();

  
  
  
  
  DWORD code = exinfo->ExceptionRecord->ExceptionCode;
  LONG action;
  if (code != EXCEPTION_BREAKPOINT && code != EXCEPTION_SINGLE_STEP &&
      current_handler->WriteMinidumpOnHandlerThread(exinfo, NULL)) {
    
    
    
    
    
    
    
    action = EXCEPTION_EXECUTE_HANDLER;
  } else {
    
    
    
    
    
    
    
    
    if (current_handler->previous_filter_) {
      action = current_handler->previous_filter_(exinfo);
    } else {
      action = EXCEPTION_CONTINUE_SEARCH;
    }
  }

  return action;
}

#if _MSC_VER >= 1400  

void ExceptionHandler::HandleInvalidParameter(const wchar_t *expression,
                                              const wchar_t *function,
                                              const wchar_t *file,
                                              unsigned int line,
                                              uintptr_t reserved) {
  
  
  AutoExceptionHandler auto_exception_handler;
  ExceptionHandler *current_handler = auto_exception_handler.get_handler();

  MDRawAssertionInfo assertion;
  memset(&assertion, 0, sizeof(assertion));
  _snwprintf_s(reinterpret_cast<wchar_t *>(assertion.expression),
               sizeof(assertion.expression) / sizeof(assertion.expression[0]),
               _TRUNCATE, L"%s", expression);
  _snwprintf_s(reinterpret_cast<wchar_t *>(assertion.function),
               sizeof(assertion.function) / sizeof(assertion.function[0]),
               _TRUNCATE, L"%s", function);
  _snwprintf_s(reinterpret_cast<wchar_t *>(assertion.file),
               sizeof(assertion.file) / sizeof(assertion.file[0]),
               _TRUNCATE, L"%s", file);
  assertion.line = line;
  assertion.type = MD_ASSERTION_INFO_TYPE_INVALID_PARAMETER;

  if (!current_handler->WriteMinidumpOnHandlerThread(NULL, &assertion)) {
    if (current_handler->previous_iph_) {
      
      
      current_handler->previous_iph_(expression, function, file, line, reserved);
    } else {
      
      
      
      
      
      
      
      
      
#ifdef _DEBUG
      _invalid_parameter(expression, function, file, line, reserved);
#else  
      _invalid_parameter_noinfo();
#endif  
    }
  }

  
  
  
  exit(0);
}
#endif  


void ExceptionHandler::HandlePureVirtualCall() {
  AutoExceptionHandler auto_exception_handler;
  ExceptionHandler *current_handler = auto_exception_handler.get_handler();

  MDRawAssertionInfo assertion;
  memset(&assertion, 0, sizeof(assertion));
  assertion.type = MD_ASSERTION_INFO_TYPE_PURE_VIRTUAL_CALL;

  if (!current_handler->WriteMinidumpOnHandlerThread(NULL, &assertion)) {
    if (current_handler->previous_pch_) {
      
      
      current_handler->previous_pch_();
    } else {
      
      
      return;
    }
  }

  
  
  
  exit(0);
}

bool ExceptionHandler::WriteMinidumpOnHandlerThread(
    EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion) {
  EnterCriticalSection(&handler_critical_section_);

  
  requesting_thread_id_ = GetCurrentThreadId();
  exception_info_ = exinfo;
  assertion_ = assertion;

  
  ReleaseSemaphore(handler_start_semaphore_, 1, NULL);

  
  WaitForSingleObject(handler_finish_semaphore_, INFINITE);
  bool status = handler_return_value_;

  
  requesting_thread_id_ = 0;
  exception_info_ = NULL;
  assertion_ = NULL;

  LeaveCriticalSection(&handler_critical_section_);

  return status;
}

bool ExceptionHandler::WriteMinidump() {
  return WriteMinidumpForException(NULL);
}

bool ExceptionHandler::WriteMinidumpForException(EXCEPTION_POINTERS *exinfo) {
  bool success = WriteMinidumpOnHandlerThread(exinfo, NULL);
  UpdateNextID();
  return success;
}


bool ExceptionHandler::WriteMinidump(const wstring &dump_path,
                                     MinidumpCallback callback,
                                     void *callback_context) {
  ExceptionHandler handler(dump_path, NULL, callback, callback_context,
                           HANDLER_NONE);
  return handler.WriteMinidump();
}

bool ExceptionHandler::WriteMinidumpWithException(
    DWORD requesting_thread_id,
    EXCEPTION_POINTERS *exinfo,
    MDRawAssertionInfo *assertion) {
  
  
  
  
  
  
  if (filter_ && !filter_(callback_context_, exinfo, assertion)) {
    return false;
  }

  bool success = false;
  if (minidump_write_dump_) {
    HANDLE dump_file = CreateFile(next_minidump_path_c_,
                                  GENERIC_WRITE,
                                  0,  
                                  NULL,
                                  CREATE_NEW,  
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    if (dump_file != INVALID_HANDLE_VALUE) {
      MINIDUMP_EXCEPTION_INFORMATION except_info;
      except_info.ThreadId = requesting_thread_id;
      except_info.ExceptionPointers = exinfo;
      except_info.ClientPointers = FALSE;

      
      
      
      
      
      
      MDRawBreakpadInfo breakpad_info;
      breakpad_info.validity = MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID |
                             MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID;
      breakpad_info.dump_thread_id = GetCurrentThreadId();
      breakpad_info.requesting_thread_id = requesting_thread_id;

      
      MINIDUMP_USER_STREAM user_stream_array[2];
      user_stream_array[0].Type = MD_BREAKPAD_INFO_STREAM;
      user_stream_array[0].BufferSize = sizeof(breakpad_info);
      user_stream_array[0].Buffer = &breakpad_info;

      MINIDUMP_USER_STREAM_INFORMATION user_streams;
      user_streams.UserStreamCount = 1;
      user_streams.UserStreamArray = user_stream_array;

      if (assertion) {
        user_stream_array[1].Type = MD_ASSERTION_INFO_STREAM;
        user_stream_array[1].BufferSize = sizeof(MDRawAssertionInfo);
        user_stream_array[1].Buffer = assertion;
        ++user_streams.UserStreamCount;
      }

      
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
    success = callback_(dump_path_c_, next_minidump_id_c_, callback_context_,
                        exinfo, assertion, success);
  }

  return success;
}

void ExceptionHandler::UpdateNextID() {
  GUID id;
  CoCreateGuid(&id);
  next_minidump_id_ = GUIDString::GUIDToWString(&id);
  next_minidump_id_c_ = next_minidump_id_.c_str();

  wchar_t minidump_path[MAX_PATH];
  swprintf(minidump_path, MAX_PATH, L"%s\\%s.dmp",
           dump_path_c_, next_minidump_id_c_);

  
  minidump_path[MAX_PATH - 1] = L'\0';

  next_minidump_path_ = minidump_path;
  next_minidump_path_c_ = next_minidump_path_.c_str();
}

}  
