






























#ifndef CLIENT_SOLARIS_HANDLER_EXCEPTION_HANDLER_H__
#define CLIENT_SOLARIS_HANDLER_EXCEPTION_HANDLER_H__

#include <map>
#include <string>
#include <vector>

#include "client/solaris/handler/minidump_generator.h"

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
  }

  
  
  bool WriteMinidump();

  
  
  static bool WriteMinidump(const string &dump_path,
                            MinidumpCallback callback,
                            void *callback_context);

 private:
  
  void SetupHandler();
  
  void SetupHandler(int signo);
  
  void TeardownHandler(int signo);
  
  void TeardownAllHandlers();

  
  static void* ExceptionHandlerThreadMain(void *lpParameter);

  
  static void HandleException(int signo);

  
  
  
  
  
  
  
  bool InternalWriteMinidump(int signo, uintptr_t sighandler_ebp,
                             ucontext_t **sig_ctx);

 private:
  
  FilterCallback filter_;
  MinidumpCallback callback_;
  void *callback_context_;

  
  
  string dump_path_;
  
  
  const char *dump_path_c_;

  
  
  bool installed_handler_;

  
  typedef void (*sighandler_t)(int);
  std::map<int, sighandler_t> old_handlers_;

  
  
  
  static std::vector<ExceptionHandler *> *handler_stack_;
  
  static int handler_stack_index_;
  static pthread_mutex_t handler_stack_mutex_;

  
  MinidumpGenerator minidump_generator_;

  
  explicit ExceptionHandler(const ExceptionHandler &);
  void operator=(const ExceptionHandler &);
};

}  

#endif  
