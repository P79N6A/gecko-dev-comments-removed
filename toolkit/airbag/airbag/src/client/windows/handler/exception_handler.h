

























































#ifndef CLIENT_WINDOWS_HANDLER_EXCEPTION_HANDLER_H__
#define CLIENT_WINDOWS_HANDLER_EXCEPTION_HANDLER_H__

#include <Windows.h>
#include <DbgHelp.h>

#pragma warning( push )

#pragma warning( disable : 4530 ) 

#include <string>

namespace google_airbag {

using std::wstring;

class ExceptionHandler {
 public:
  
  
  
  
  typedef void (*MinidumpCallback)(const wstring &minidump_id,
                                   void *context, bool succeeded);

  
  
  
  
  
  
  ExceptionHandler(const wstring &dump_path, MinidumpCallback callback,
                   void *callback_context, bool install_handler);
  ~ExceptionHandler();

  
  wstring dump_path() const { return dump_path_; }
  void set_dump_path(const wstring &dump_path) { dump_path_ = dump_path; }

  
  
  bool WriteMinidump();

  
  
  static bool WriteMinidump(const wstring &dump_path,
                            MinidumpCallback callback, void *callback_context);

 private:
  
  
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

  
  
  
  
  
  
  
  bool WriteMinidumpOnHandlerThread(EXCEPTION_POINTERS *exinfo);

  
  
  
  
  
  bool WriteMinidumpWithException(DWORD requesting_thread_id,
                                  EXCEPTION_POINTERS *exinfo);

  
  void UpdateNextID();

  MinidumpCallback callback_;
  void *callback_context_;

  wstring dump_path_;
  wstring next_minidump_id_;

  HMODULE dbghelp_module_;
  MiniDumpWriteDump_type minidump_write_dump_;

  ExceptionHandler *previous_handler_;  
  LPTOP_LEVEL_EXCEPTION_FILTER previous_filter_;

  
  static ExceptionHandler *current_handler_;

  
  static HANDLE handler_thread_;

  
  
  static CRITICAL_SECTION handler_critical_section_;

  
  
  
  
  
  static HANDLE handler_start_semaphore_;
  static HANDLE handler_finish_semaphore_;

  
  

  
  
  
  static ExceptionHandler *requesting_handler_;

  
  
  static DWORD requesting_thread_id_;

  
  
  static EXCEPTION_POINTERS *exception_info_;

  
  
  static bool handler_return_value_;

  
  explicit ExceptionHandler(const ExceptionHandler &);
  void operator=(const ExceptionHandler &);
};

}  

#pragma warning( pop )

#endif  
