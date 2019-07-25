




























#ifndef CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H_
#define CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H_

#include <vector>
#include <string>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>

#include "client/linux/android_ucontext.h"
#include "client/linux/crash_generation/crash_generation_client.h"
#include "processor/scoped_ptr.h"

struct sigaction;

namespace google_breakpad {

class ExceptionHandler;

























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

  
  
  
  
  
  ExceptionHandler(const std::string& dump_path,
                   FilterCallback filter, MinidumpCallback callback,
                   void* callback_context,
                   bool install_handler,
                   const int server_fd);

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

  
  
  bool WriteMinidump(bool write_exception_stream);

  
  
  static bool WriteMinidump(const std::string &dump_path,
                            MinidumpCallback callback,
                            void *callback_context);

  
  
  static bool WriteMinidump(const std::string &dump_path,
                            bool write_exception_stream,
                            MinidumpCallback callback,
                            void* callback_context);

  
  
  
  
  
  
  
  
  
  
  
  static bool WriteMinidumpForChild(pid_t child,
                                    pid_t child_blamed_thread,
                                    const std::string &dump_path,
                                    MinidumpCallback callback,
                                    void *callback_context);

  
  
  struct CrashContext {
    siginfo_t siginfo;
    pid_t tid;  
    struct ucontext context;
#if !defined(__ARM_EABI__)
    
    struct _libc_fpstate float_state;
#endif
  };

  
  bool IsOutOfProcess() const {
      return crash_generation_client_.get() != NULL;
  }

 private:
  void Init(const std::string &dump_path,
            const int server_fd);
  bool InstallHandlers();
  void UninstallHandlers();
  void PreresolveSymbols();
  bool GenerateDump(CrashContext *context);
  void SendContinueSignalToChild();
  void WaitForContinueSignal();

  void UpdateNextID();
  static void SignalHandler(int sig, siginfo_t* info, void* uc);
  bool HandleSignal(int sig, siginfo_t* info, void* uc);
  static int ThreadEntry(void* arg);
  bool DoDump(pid_t crashing_process, const void* context,
              size_t context_size);

  const FilterCallback filter_;
  const MinidumpCallback callback_;
  void* const callback_context_;

  scoped_ptr<CrashGenerationClient> crash_generation_client_;

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

  
  std::vector<std::pair<int, struct sigaction *> > old_handlers_;

  
  
  
  
  
  int fdes[2];
};

}  

#endif  
