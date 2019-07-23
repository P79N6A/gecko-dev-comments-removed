

























































#ifndef CLIENT_WINDOWS_HANDLER_EXCEPTION_HANDLER_H__
#define CLIENT_WINDOWS_HANDLER_EXCEPTION_HANDLER_H__

#include <stdlib.h>
#include <Windows.h>
#include <DbgHelp.h>

#pragma warning( push )

#pragma warning( disable : 4530 ) 

#include <string>
#include <vector>

#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {

using std::vector;
using std::wstring;

class ExceptionHandler {
 public:
  
  
  
  
  
  
  
  
  
  
  
  typedef bool (*FilterCallback)(void *context, EXCEPTION_POINTERS *exinfo,
                                 MDRawAssertionInfo *assertion);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef bool (*MinidumpCallback)(const wchar_t *dump_path,
                                   const wchar_t *minidump_id,
                                   void *context,
                                   EXCEPTION_POINTERS *exinfo,
                                   MDRawAssertionInfo *assertion,
                                   bool succeeded);

  
  
  
  
  
  
  enum HandlerType {
    HANDLER_NONE = 0,
    HANDLER_EXCEPTION = 1 << 0,          
    HANDLER_INVALID_PARAMETER = 1 << 1,  
    HANDLER_PURECALL = 1 << 2,           
    HANDLER_ALL = HANDLER_EXCEPTION |
                  HANDLER_INVALID_PARAMETER |
                  HANDLER_PURECALL
  };

  
  
  
  
  
  
  ExceptionHandler(const wstring &dump_path,
                   FilterCallback filter,
                   MinidumpCallback callback,
                   void *callback_context,
                   int handler_types);
  ~ExceptionHandler();

  
  wstring dump_path() const { return dump_path_; }
  void set_dump_path(const wstring &dump_path) {
    dump_path_ = dump_path;
    dump_path_c_ = dump_path_.c_str();
    UpdateNextID();  
  }

  
  
  bool WriteMinidump();

  
  
  bool WriteMinidumpForException(EXCEPTION_POINTERS *exinfo);

  
  
  static bool WriteMinidump(const wstring &dump_path,
                            MinidumpCallback callback, void *callback_context);

 private:
  friend class AutoExceptionHandler;

  
  
  typedef BOOL (WINAPI *MiniDumpWriteDump_type)(
      HANDLE hProcess,
      DWORD dwPid,
      HANDLE hFile,
      MINIDUMP_TYPE DumpType,
      CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
      CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
      CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

  
  static DWORD WINAPI ExceptionHandlerThreadMain(void *lpParameter);

  
  
  static LONG WINAPI HandleException(EXCEPTION_POINTERS *exinfo);

#if _MSC_VER >= 1400  
  
  
  
  
  
  static void HandleInvalidParameter(const wchar_t *expression,
                                     const wchar_t *function,
                                     const wchar_t *file,
                                     unsigned int line,
                                     uintptr_t reserved);
#endif  

  
  
  static void HandlePureVirtualCall();

  
  
  
  
  
  
  
  
  
  bool WriteMinidumpOnHandlerThread(EXCEPTION_POINTERS *exinfo,
                                    MDRawAssertionInfo *assertion);

  
  
  
  
  
  bool WriteMinidumpWithException(DWORD requesting_thread_id,
                                  EXCEPTION_POINTERS *exinfo,
                                  MDRawAssertionInfo *assertion);

  
  
  void UpdateNextID();

  FilterCallback filter_;
  MinidumpCallback callback_;
  void *callback_context_;

  
  
  wstring dump_path_;

  
  wstring next_minidump_id_;

  
  
  wstring next_minidump_path_;

  
  
  
  
  
  
  const wchar_t *dump_path_c_;
  const wchar_t *next_minidump_id_c_;
  const wchar_t *next_minidump_path_c_;

  HMODULE dbghelp_module_;
  MiniDumpWriteDump_type minidump_write_dump_;

  
  
  int handler_types_;

  
  
  
  
  LPTOP_LEVEL_EXCEPTION_FILTER previous_filter_;

#if _MSC_VER >= 1400  
  
  
  
  
  _invalid_parameter_handler previous_iph_;
#endif  

  
  
  _purecall_handler previous_pch_;

  
  HANDLE handler_thread_;

  
  
  CRITICAL_SECTION handler_critical_section_;

  
  
  
  
  
  HANDLE handler_start_semaphore_;
  HANDLE handler_finish_semaphore_;

  
  

  
  
  DWORD requesting_thread_id_;

  
  
  EXCEPTION_POINTERS *exception_info_;

  
  
  MDRawAssertionInfo *assertion_;

  
  
  bool handler_return_value_;

  
  
  
  
  
  static vector<ExceptionHandler *> *handler_stack_;

  
  
  
  
  static LONG handler_stack_index_;

  
  
  static CRITICAL_SECTION handler_stack_critical_section_;

  
  static bool handler_stack_critical_section_initialized_;

  
  explicit ExceptionHandler(const ExceptionHandler &);
  void operator=(const ExceptionHandler &);
};

}  

#pragma warning( pop )

#endif  
