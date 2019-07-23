






























#ifndef CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H__
#define CLIENT_LINUX_HANDLER_EXCEPTION_HANDLER_H__

#include <pthread.h>

#include <map>
#include <string>
#include <signal.h>
#include <vector>

#include "client/linux/handler/minidump_generator.h"


struct sigcontex;

namespace google_breakpad {

using std::string;



























class ExceptionHandler {
 public:
  
  
  
  
  
  
  
  
  
  typedef bool (*FilterCallback)(void *context);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef bool (*MinidumpCallback)(const char *dump_path,
                                   const char *minidump_id,
                                   void *context,
                                   bool succeeded);

  
  
  
  
  
  
  
  
  ExceptionHandler(const string &dump_path,
                   FilterCallback filter, MinidumpCallback callback,
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

  
  
  static bool WriteMinidump(const string &dump_path,
                            MinidumpCallback callback,
                            void *callback_context);

 private:
  
  void SetupHandler();
  
  void SetupHandler(int signo);
  
  void TeardownHandler(int signo);
  
  void TeardownHandler(int signo, struct sigaction *old);
  
  void TeardownAllHandler();

  
  static void HandleException(int signo);

  
  
  
  
  
  
  bool InternalWriteMinidump(int signo, uintptr_t sighandler_ebp,
                             struct sigcontext **sig_ctx);

  
  
  void UpdateNextID();

 private:
  FilterCallback filter_;
  MinidumpCallback callback_;
  void *callback_context_;

  
  
  string dump_path_;

  
  string next_minidump_id_;

  
  
  string next_minidump_path_;

  
  
  
  const char *dump_path_c_;
  const char *next_minidump_id_c_;
  const char *next_minidump_path_c_;

  
  
  bool installed_handler_;

  
  
  
  static std::vector<ExceptionHandler *> *handler_stack_;
  
  static int handler_stack_index_;
  static pthread_mutex_t handler_stack_mutex_;

  
  MinidumpGenerator minidump_generator_;

  
  explicit ExceptionHandler(const ExceptionHandler &);
  void operator=(const ExceptionHandler &);

  
  struct sigaction act_;


  
  
  
  
  struct sigaction old_actions_[NSIG];
};

}  

#endif  
