




























#ifndef CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H_
#define CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H_

#include <vector>
#include <string>

#include <signal.h>

namespace google_breakpad {

























class ExceptionHandler {
 public:
  
  
  
  
  
  
  
  
  
  typedef bool (*FilterCallback)(void *context);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef bool (*MinidumpCallback)(const char *dump_path,
                                   const char *minidump_id,
                                   void *context,
                                   bool succeeded);

  
  
  
  
  
  typedef bool (*HandlerCallback)(const void* crash_context,
                                  size_t crash_context_size,
                                  void* context);

  
  
  
  
  
  
  
  
  ExceptionHandler(const std::string &dump_path,
                   FilterCallback filter, MinidumpCallback callback,
                   void *callback_context,
                   bool install_handler);
  ~ExceptionHandler();

  
  std::string dump_path() const { return dump_path_; }
  void set_dump_path(const std::string &dump_path) {
    dump_path_ = dump_path;
    dump_path_c_ = dump_path_.c_str();
    UpdateNextID();
  }

  void set_crash_handler(HandlerCallback callback) {
    crash_handler_ = callback;
  }

  
  
  bool WriteMinidump();

  
  
  static bool WriteMinidump(const std::string &dump_path,
                            MinidumpCallback callback,
                            void *callback_context);

  
  
  struct CrashContext {
    siginfo_t siginfo;
    pid_t tid;  
    struct ucontext context;
    struct _libc_fpstate float_state;
  };

 private:
  bool InstallHandlers();
  void UninstallHandlers();
  void PreresolveSymbols();

  void UpdateNextID();
  static void SignalHandler(int sig, siginfo_t* info, void* uc);
  bool HandleSignal(int sig, siginfo_t* info, void* uc);
  static int ThreadEntry(void* arg);
  bool DoDump(pid_t crashing_process, const void* context,
              size_t context_size);

  const FilterCallback filter_;
  const MinidumpCallback callback_;
  void* const callback_context_;

  std::string dump_path_;
  std::string next_minidump_path_;
  std::string next_minidump_id_;

  
  
  
  const char* dump_path_c_;
  const char* next_minidump_path_c_;
  const char* next_minidump_id_c_;

  const bool handler_installed_;
  void* signal_stack;  
  HandlerCallback crash_handler_;

  
  
  
  static std::vector<ExceptionHandler*> *handler_stack_;
  
  static unsigned handler_stack_index_;
  static pthread_mutex_t handler_stack_mutex_;

  
  
  std::vector<std::pair<int, void *> > old_handlers_;
};

}  

#endif  
