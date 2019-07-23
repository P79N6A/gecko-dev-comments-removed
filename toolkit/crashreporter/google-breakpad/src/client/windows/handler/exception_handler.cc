




























#include <ObjBase.h>
#include <winternl.h>
#include <psapi.h>

#include <cassert>
#include <cstdio>

#include "common/windows/string_utils-inl.h"

#include "client/windows/common/ipc_protocol.h"
#include "client/windows/handler/exception_handler.h"
#include "common/windows/guid_string.h"

namespace {


bool GetProcIdViaGetProcessId(HANDLE process, DWORD* id) {
  
  typedef DWORD (WINAPI *GetProcessIdFunction)(HANDLE);
  static GetProcessIdFunction GetProcessIdPtr = NULL;
  static bool initialize_get_process_id = true;
  if (initialize_get_process_id) {
    initialize_get_process_id = false;
    HMODULE kernel32_handle = GetModuleHandle(L"kernel32.dll");
    if (!kernel32_handle) {
      return false;
    }
    GetProcessIdPtr = reinterpret_cast<GetProcessIdFunction>(GetProcAddress(
        kernel32_handle, "GetProcessId"));
  }
  if (!GetProcessIdPtr)
    return false;
  
  *id = (*GetProcessIdPtr)(process);
  return true;
}


bool GetProcIdViaNtQueryInformationProcess(HANDLE process, DWORD* id) {
  
  typedef NTSTATUS (WINAPI *NtQueryInformationProcessFunction)(
      HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
  static NtQueryInformationProcessFunction NtQueryInformationProcessPtr = NULL;
  static bool initialize_query_information_process = true;
  if (initialize_query_information_process) {
    initialize_query_information_process = false;
    
    
    HMODULE ntdll_handle = GetModuleHandle(L"ntdll.dll");
    if (!ntdll_handle) {
      return false;
    }
    NtQueryInformationProcessPtr =
        reinterpret_cast<NtQueryInformationProcessFunction>(GetProcAddress(
            ntdll_handle, "NtQueryInformationProcess"));
  }
  if (!NtQueryInformationProcessPtr)
    return false;
  
  PROCESS_BASIC_INFORMATION info;
  ULONG bytes_returned;
  NTSTATUS status = (*NtQueryInformationProcessPtr)(process,
                                                    ProcessBasicInformation,
                                                    &info, sizeof info,
                                                    &bytes_returned);
  if (!SUCCEEDED(status) || (bytes_returned != (sizeof info)))
    return false;

  *id = static_cast<DWORD>(info.UniqueProcessId);
  return true;
}

DWORD GetProcId(HANDLE process) {
  
  HANDLE current_process = GetCurrentProcess();
  HANDLE process_with_query_rights;
  if (DuplicateHandle(current_process, process, current_process,
                      &process_with_query_rights, PROCESS_QUERY_INFORMATION,
                      false, 0)) {
    
    
    DWORD id;
    bool success =
        GetProcIdViaGetProcessId(process_with_query_rights, &id) ||
        GetProcIdViaNtQueryInformationProcess(process_with_query_rights, &id);
    CloseHandle(process_with_query_rights);
    if (success)
      return id;
  }

  
  return 0;
}

} 

namespace google_breakpad {

static const int kWaitForHandlerThreadMs = 60000;
static const int kExceptionHandlerThreadInitialStackSize = 64 * 1024;

vector<ExceptionHandler*>* ExceptionHandler::handler_stack_ = NULL;
LONG ExceptionHandler::handler_stack_index_ = 0;
CRITICAL_SECTION ExceptionHandler::handler_stack_critical_section_;
volatile LONG ExceptionHandler::instance_count_ = 0;

ExceptionHandler::ExceptionHandler(const wstring& dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void* callback_context,
                                   int handler_types,
                                   MINIDUMP_TYPE dump_type,
                                   const wchar_t* pipe_name,
                                   const CustomClientInfo* custom_info) {
  Initialize(dump_path,
             filter,
             callback,
             callback_context,
             handler_types,
             dump_type,
             pipe_name,
             custom_info);
}

ExceptionHandler::ExceptionHandler(const wstring &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void* callback_context,
                                   int handler_types) {
  Initialize(dump_path,
             filter,
             callback,
             callback_context,
             handler_types,
             MiniDumpNormal,
             NULL,
             NULL);
}

void ExceptionHandler::Initialize(const wstring& dump_path,
                                  FilterCallback filter,
                                  MinidumpCallback callback,
                                  void* callback_context,
                                  int handler_types,
                                  MINIDUMP_TYPE dump_type,
                                  const wchar_t* pipe_name,
                                  const CustomClientInfo* custom_info) {
  LONG instance_count = InterlockedIncrement(&instance_count_);
  filter_ = filter;
  callback_ = callback;
  callback_context_ = callback_context;
  dump_path_c_ = NULL;
  next_minidump_id_c_ = NULL;
  next_minidump_path_c_ = NULL;
  dbghelp_module_ = NULL;
  minidump_write_dump_ = NULL;
  dump_type_ = dump_type;
  rpcrt4_module_ = NULL;
  uuid_create_ = NULL;
  handler_types_ = handler_types;
  previous_filter_ = NULL;
#if _MSC_VER >= 1400  
  previous_iph_ = NULL;
#endif  
  previous_pch_ = NULL;
  handler_thread_ = NULL;
  is_shutdown_ = false;
  handler_start_semaphore_ = NULL;
  handler_finish_semaphore_ = NULL;
  requesting_thread_id_ = 0;
  exception_info_ = NULL;
  assertion_ = NULL;
  handler_return_value_ = false;
  handle_debug_exceptions_ = false;

  
  if (pipe_name != NULL) {
    scoped_ptr<CrashGenerationClient> client(
        new CrashGenerationClient(pipe_name,
                                  dump_type_,
                                  custom_info));

    
    
    if (client->Register()) {
      crash_generation_client_.reset(client.release());
    }
  }

  if (!IsOutOfProcess()) {
    
    
    

    
    
    
    
    
    InitializeCriticalSection(&handler_critical_section_);
    handler_start_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);
    assert(handler_start_semaphore_ != NULL);

    handler_finish_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);
    assert(handler_finish_semaphore_ != NULL);

    
    if (handler_finish_semaphore_ != NULL && handler_start_semaphore_ != NULL) {
      DWORD thread_id;
      handler_thread_ = CreateThread(NULL,         
                                     kExceptionHandlerThreadInitialStackSize,
                                     ExceptionHandlerThreadMain,
                                     this,         
                                     0,            
                                     &thread_id);
      assert(handler_thread_ != NULL);
    }

    dbghelp_module_ = LoadLibrary(L"dbghelp.dll");
    if (dbghelp_module_) {
      minidump_write_dump_ = reinterpret_cast<MiniDumpWriteDump_type>(
          GetProcAddress(dbghelp_module_, "MiniDumpWriteDump"));
    }

    
    
    
    rpcrt4_module_ = LoadLibrary(L"rpcrt4.dll");
    if (rpcrt4_module_) {
      uuid_create_ = reinterpret_cast<UuidCreate_type>(
          GetProcAddress(rpcrt4_module_, "UuidCreate"));
    }

    
    
    set_dump_path(dump_path);
  }

  
  
  
  
  
  
  
  
  

  
  if (instance_count == 1) {
    InitializeCriticalSection(&handler_stack_critical_section_);
  }

  if (handler_types != HANDLER_NONE) {
    EnterCriticalSection(&handler_stack_critical_section_);

    
    
    if (!handler_stack_) {
      handler_stack_ = new vector<ExceptionHandler*>();
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

  if (rpcrt4_module_) {
    FreeLibrary(rpcrt4_module_);
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
      vector<ExceptionHandler*>::iterator iterator = handler_stack_->begin();
      while (iterator != handler_stack_->end()) {
        if (*iterator == this) {
          iterator = handler_stack_->erase(iterator);
        } else {
          ++iterator;
        }
      }
    }

    if (handler_stack_->empty()) {
      
      
      delete handler_stack_;
      handler_stack_ = NULL;
    }

    LeaveCriticalSection(&handler_stack_critical_section_);
  }

  
  
  if (!IsOutOfProcess()) {
#ifdef BREAKPAD_NO_TERMINATE_THREAD
    
    
    
    
    
    
    is_shutdown_ = true;
    ReleaseSemaphore(handler_start_semaphore_, 1, NULL);
    WaitForSingleObject(handler_thread_, kWaitForHandlerThreadMs);
#else
    TerminateThread(handler_thread_, 1);
#endif  

    CloseHandle(handler_thread_);
    handler_thread_ = NULL;
    DeleteCriticalSection(&handler_critical_section_);
    CloseHandle(handler_start_semaphore_);
    CloseHandle(handler_finish_semaphore_);
  }

  
  
  
  
  
  
  if (InterlockedDecrement(&instance_count_) == 0) {
    DeleteCriticalSection(&handler_stack_critical_section_);
  }
}


DWORD ExceptionHandler::ExceptionHandlerThreadMain(void* lpParameter) {
  ExceptionHandler* self = reinterpret_cast<ExceptionHandler *>(lpParameter);
  assert(self);
  assert(self->handler_start_semaphore_ != NULL);
  assert(self->handler_finish_semaphore_ != NULL);

  while (true) {
    if (WaitForSingleObject(self->handler_start_semaphore_, INFINITE) ==
        WAIT_OBJECT_0) {
      
      if (self->is_shutdown_) {
        
        break;
      } else {
        self->handler_return_value_ =
            self->WriteMinidumpWithException(self->requesting_thread_id_,
                                             self->exception_info_,
                                             self->assertion_);
      }

      
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

  ExceptionHandler* get_handler() const { return handler_; }

 private:
  ExceptionHandler* handler_;
};


LONG ExceptionHandler::HandleException(EXCEPTION_POINTERS* exinfo) {
  AutoExceptionHandler auto_exception_handler;
  ExceptionHandler* current_handler = auto_exception_handler.get_handler();

  
  
  
  
  
  DWORD code = exinfo->ExceptionRecord->ExceptionCode;
  LONG action;
  bool is_debug_exception = (code == EXCEPTION_BREAKPOINT) ||
                            (code == EXCEPTION_SINGLE_STEP);

  bool success = false;

  if (!is_debug_exception ||
      current_handler->get_handle_debug_exceptions()) {
    
    
    

    
    
    if (current_handler->IsOutOfProcess()) {
      success = current_handler->WriteMinidumpWithException(
          GetCurrentThreadId(),
          exinfo,
          NULL);
    } else {
      success = current_handler->WriteMinidumpOnHandlerThread(exinfo, NULL);
    }
  }

  
  
  
  
  
  
  
  if (success) {
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

void ExceptionHandler::HandleInvalidParameter(const wchar_t* expression,
                                              const wchar_t* function,
                                              const wchar_t* file,
                                              unsigned int line,
                                              uintptr_t reserved) {
  
  
  AutoExceptionHandler auto_exception_handler;
  ExceptionHandler* current_handler = auto_exception_handler.get_handler();

  MDRawAssertionInfo assertion;
  memset(&assertion, 0, sizeof(assertion));
  _snwprintf_s(reinterpret_cast<wchar_t*>(assertion.expression),
               sizeof(assertion.expression) / sizeof(assertion.expression[0]),
               _TRUNCATE, L"%s", expression);
  _snwprintf_s(reinterpret_cast<wchar_t*>(assertion.function),
               sizeof(assertion.function) / sizeof(assertion.function[0]),
               _TRUNCATE, L"%s", function);
  _snwprintf_s(reinterpret_cast<wchar_t*>(assertion.file),
               sizeof(assertion.file) / sizeof(assertion.file[0]),
               _TRUNCATE, L"%s", file);
  assertion.line = line;
  assertion.type = MD_ASSERTION_INFO_TYPE_INVALID_PARAMETER;

  bool success = false;
  
  
  if (current_handler->IsOutOfProcess()) {
    success = current_handler->WriteMinidumpWithException(
        GetCurrentThreadId(),
        NULL,
        &assertion);
  } else {
    success = current_handler->WriteMinidumpOnHandlerThread(NULL, &assertion);
  }

  if (!success) {
    if (current_handler->previous_iph_) {
      
      
      current_handler->previous_iph_(expression,
                                     function,
                                     file,
                                     line,
                                     reserved);
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
  ExceptionHandler* current_handler = auto_exception_handler.get_handler();

  MDRawAssertionInfo assertion;
  memset(&assertion, 0, sizeof(assertion));
  assertion.type = MD_ASSERTION_INFO_TYPE_PURE_VIRTUAL_CALL;

  bool success = false;
  
  

  if (current_handler->IsOutOfProcess()) {
    success = current_handler->WriteMinidumpWithException(
        GetCurrentThreadId(),
        NULL,
        &assertion);
  } else {
    success = current_handler->WriteMinidumpOnHandlerThread(NULL, &assertion);
  }

  if (!success) {
    if (current_handler->previous_pch_) {
      
      
      current_handler->previous_pch_();
    } else {
      
      
      return;
    }
  }

  
  
  
  exit(0);
}

bool ExceptionHandler::WriteMinidumpOnHandlerThread(
    EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion) {
  EnterCriticalSection(&handler_critical_section_);

  
  
  if (handler_thread_ == NULL) {
    LeaveCriticalSection(&handler_critical_section_);
    return false;
  }

  
  assert(handler_start_semaphore_ != NULL);
  assert(handler_finish_semaphore_ != NULL);

  
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

bool ExceptionHandler::WriteMinidumpForException(EXCEPTION_POINTERS* exinfo) {
  
  
  if (IsOutOfProcess()) {
    return WriteMinidumpWithException(GetCurrentThreadId(),
                                      exinfo,
                                      NULL);
  }

  bool success = WriteMinidumpOnHandlerThread(exinfo, NULL);
  UpdateNextID();
  return success;
}


bool ExceptionHandler::WriteMinidump(const wstring &dump_path,
                                     MinidumpCallback callback,
                                     void* callback_context) {
  ExceptionHandler handler(dump_path, NULL, callback, callback_context,
                           HANDLER_NONE);
  return handler.WriteMinidump();
}


bool ExceptionHandler::WriteMinidumpForChild(HANDLE child,
					     const wstring &dump_path,
					     MinidumpCallback callback,
					     void *callback_context) {
  DWORD childId = GetProcId(child);
  if (0 == childId)
    return false;

  ExceptionHandler handler(dump_path, NULL, callback, callback_context,
                           HANDLER_NONE);
  bool success = handler.WriteMinidumpWithExceptionForProcess(
      0, NULL, NULL, child, childId, false);

  if (callback) {
    success = callback(handler.dump_path_c_, handler.next_minidump_id_c_,
		       callback_context, NULL, NULL, success);
  }

  return success;
}

bool ExceptionHandler::WriteMinidumpWithException(
    DWORD requesting_thread_id,
    EXCEPTION_POINTERS* exinfo,
    MDRawAssertionInfo* assertion) {
  
  
  
  
  
  
  if (filter_ && !filter_(callback_context_, exinfo, assertion)) {
    return false;
  }

  bool success = false;
  if (IsOutOfProcess()) {
    
    
    if (!assertion) {
      success = crash_generation_client_->RequestDump(exinfo);
    } else {
      success = crash_generation_client_->RequestDump(assertion);
    }
  } else {
    success = WriteMinidumpWithExceptionForProcess(requesting_thread_id,
                                                   exinfo,
                                                   assertion,
                                                   GetCurrentProcess(),
                                                   GetCurrentProcessId(),
                                                   true);
  }

  if (callback_) {
    
    
    
    
    success = callback_(dump_path_c_, next_minidump_id_c_, callback_context_,
                        exinfo, assertion, success);
  }

  return success;
}

bool ExceptionHandler::WriteMinidumpWithExceptionForProcess(
    DWORD requesting_thread_id,
    EXCEPTION_POINTERS* exinfo,
    MDRawAssertionInfo* assertion,
    HANDLE process,
    DWORD processId,
    bool write_requester_stream) {
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

      
      
      MINIDUMP_USER_STREAM user_stream_array[2];
      MINIDUMP_USER_STREAM_INFORMATION user_streams;
      user_streams.UserStreamCount = 0;
      user_streams.UserStreamArray = user_stream_array;

      if (write_requester_stream) {
        
        
        
        
        
        
        
        
        MDRawBreakpadInfo breakpad_info;
        breakpad_info.validity = MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID |
                                 MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID;
        breakpad_info.dump_thread_id = GetCurrentThreadId();
        breakpad_info.requesting_thread_id = requesting_thread_id;

        int idx = user_streams.UserStreamCount;
        user_stream_array[idx].Type = MD_BREAKPAD_INFO_STREAM;
        user_stream_array[idx].BufferSize = sizeof(breakpad_info);
        user_stream_array[idx].Buffer = &breakpad_info;
        ++user_streams.UserStreamCount;
      }

      if (assertion) {
        int idx = user_streams.UserStreamCount;
        user_stream_array[idx].Type = MD_ASSERTION_INFO_STREAM;
        user_stream_array[idx].BufferSize = sizeof(MDRawAssertionInfo);
        user_stream_array[idx].Buffer = assertion;
        ++user_streams.UserStreamCount;
      }

      
      success = (minidump_write_dump_(process,
                                      processId,
                                      dump_file,
                                      dump_type_,
                                      exinfo ? &except_info : NULL,
                                      &user_streams,
                                      NULL) == TRUE);

      CloseHandle(dump_file);
    }
  }

  return success;
}

void ExceptionHandler::UpdateNextID() {
  assert(uuid_create_);
  UUID id = {0};
  if (uuid_create_) {
    uuid_create_(&id);
  }
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
