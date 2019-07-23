


































#ifndef CLIENT_MAC_HANDLER_EXCEPTION_HANDLER_H__
#define CLIENT_MAC_HANDLER_EXCEPTION_HANDLER_H__

#include <mach/mach.h>

#include <string>

namespace google_breakpad {

using std::string;

struct ExceptionParameters;

class ExceptionHandler {
 public:
  
  
  
  
  
  
  
  
  
  typedef bool (*FilterCallback)(void *context);

  
  
  
  
  
  
  
  
  typedef bool (*MinidumpCallback)(const char *dump_dir,
                                   const char *minidump_id,
                                   void *context, bool succeeded);

  
  
  
  typedef bool (*DirectCallback)( void *context,
                                  int exception_type,
                                  int exception_code,
                                  int exception_subcode,
                                  mach_port_t thread_name);

  
  
  
  
  
  
  ExceptionHandler(const string &dump_path,
                   FilterCallback filter, MinidumpCallback callback,
                   void *callback_context, bool install_handler);

  
  
  ExceptionHandler(DirectCallback callback,
                   void *callback_context,
                   bool install_handler);

  ~ExceptionHandler();

  
  string dump_path() const { return dump_path_; }
  void set_dump_path(const string &dump_path) {
    dump_path_ = dump_path;
    dump_path_c_ = dump_path_.c_str();
    UpdateNextID();  
  }

  
  
  bool WriteMinidump();

  
  
  static bool WriteMinidump(const string &dump_path, MinidumpCallback callback,
                            void *callback_context);

 private:
  
  bool InstallHandler();

  
  bool UninstallHandler(bool in_exception);

  
  
  bool Setup(bool install_handler);

  
  
  bool Teardown();

  
  
  bool SendEmptyMachMessage();

  
  bool WriteMinidumpWithException(int exception_type, int exception_code,
                                  int exception_subcode, mach_port_t thread_name);

  
  
  static void *WaitForMessage(void *exception_handler_class);

  
  explicit ExceptionHandler(const ExceptionHandler &);
  void operator=(const ExceptionHandler &);

  
  
  void UpdateNextID();

  
  
  bool SuspendThreads();
  bool ResumeThreads();

  
  string dump_path_;

  
  string next_minidump_id_;

  
  string next_minidump_path_;

  
  const char *dump_path_c_;
  const char *next_minidump_id_c_;
  const char *next_minidump_path_c_;

  
  
  FilterCallback filter_;
  MinidumpCallback callback_;
  void *callback_context_;

  
  
  DirectCallback directCallback_;

  
  pthread_t handler_thread_;

  
  
  mach_port_t handler_port_;

  
  
  ExceptionParameters *previous_;

  
  bool installed_exception_handler_;

  
  
  bool is_in_teardown_;

  
  bool last_minidump_write_result_;

  
  
  pthread_mutex_t minidump_write_mutex_;

  
  bool use_minidump_write_mutex_;
};

}  

#endif  
