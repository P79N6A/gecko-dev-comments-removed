
































































#include "client/linux/handler/exception_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/ucontext.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common/linux/linux_libc_support.h"
#include "common/linux/linux_syscall_support.h"
#include "common/linux/memory.h"
#include "client/linux/minidump_writer//minidump_writer.h"
#include "common/linux/guid_creator.h"


static int tgkill(pid_t tgid, pid_t tid, int sig) {
  syscall(__NR_tgkill, tgid, tid, sig);
  return 0;
}

namespace google_breakpad {




static const int kExceptionSignals[] = {
  SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, -1
};



std::vector<ExceptionHandler*>* ExceptionHandler::handler_stack_ = NULL;
unsigned ExceptionHandler::handler_stack_index_ = 0;
pthread_mutex_t ExceptionHandler::handler_stack_mutex_ =
    PTHREAD_MUTEX_INITIALIZER;


ExceptionHandler::ExceptionHandler(const std::string &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   bool install_handler)
    : filter_(filter),
      callback_(callback),
      callback_context_(callback_context),
      dump_path_(),
      handler_installed_(install_handler),
      crash_handler_(NULL) {
  set_dump_path(dump_path);

  if (install_handler) {
    InstallHandlers();

    pthread_mutex_lock(&handler_stack_mutex_);
      if (handler_stack_ == NULL)
        handler_stack_ = new std::vector<ExceptionHandler *>;
      handler_stack_->push_back(this);
    pthread_mutex_unlock(&handler_stack_mutex_);
  }
}


ExceptionHandler::~ExceptionHandler() {
  UninstallHandlers();
}


bool ExceptionHandler::InstallHandlers() {
  
  

  
  
  static const unsigned kSigStackSize = 8192;

  signal_stack = malloc(kSigStackSize);
  stack_t stack;
  memset(&stack, 0, sizeof(stack));
  stack.ss_sp = signal_stack;
  stack.ss_size = kSigStackSize;

  if (sigaltstack(&stack, NULL) == -1)
    return false;

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);

  
  for (unsigned i = 0; kExceptionSignals[i] != -1; ++i)
    sigaddset(&sa.sa_mask, kExceptionSignals[i]);

  sa.sa_sigaction = SignalHandler;
  sa.sa_flags = SA_ONSTACK | SA_SIGINFO;

  for (unsigned i = 0; kExceptionSignals[i] != -1; ++i) {
    struct sigaction* old = new struct sigaction;
    if (sigaction(kExceptionSignals[i], &sa, old) == -1)
      return false;
    old_handlers_.push_back(std::make_pair(kExceptionSignals[i], old));
  }
  return true;
}


void ExceptionHandler::UninstallHandlers() {
  for (unsigned i = 0; i < old_handlers_.size(); ++i) {
    struct sigaction *action =
        reinterpret_cast<struct sigaction*>(old_handlers_[i].second);
    sigaction(old_handlers_[i].first, action, NULL);
    delete action;
  }

  old_handlers_.clear();
}


void ExceptionHandler::UpdateNextID() {
  GUID guid;
  char guid_str[kGUIDStringLength + 1];
  if (CreateGUID(&guid) && GUIDToString(&guid, guid_str, sizeof(guid_str))) {
    next_minidump_id_ = guid_str;
    next_minidump_id_c_ = next_minidump_id_.c_str();

    char minidump_path[PATH_MAX];
    snprintf(minidump_path, sizeof(minidump_path), "%s/%s.dmp",
             dump_path_c_,
             guid_str);

    next_minidump_path_ = minidump_path;
    next_minidump_path_c_ = next_minidump_path_.c_str();
  }
}




void ExceptionHandler::SignalHandler(int sig, siginfo_t* info, void* uc) {
  

  pthread_mutex_lock(&handler_stack_mutex_);

  if (!handler_stack_->size()) {
    pthread_mutex_unlock(&handler_stack_mutex_);
    return;
  }

  for (int i = handler_stack_->size() - 1; i >= 0; --i) {
    if ((*handler_stack_)[i]->HandleSignal(sig, info, uc)) {
      
      
      
      break;
    }
  }

  pthread_mutex_unlock(&handler_stack_mutex_);

  
  
  
  signal(sig, SIG_DFL);
  tgkill(getpid(), sys_gettid(), sig);

  
}

struct ThreadArgument {
  pid_t pid;  
  ExceptionHandler* handler;
  const void* context;  
  size_t context_size;
};




int ExceptionHandler::ThreadEntry(void *arg) {
  const ThreadArgument *thread_arg = reinterpret_cast<ThreadArgument*>(arg);
  return thread_arg->handler->DoDump(thread_arg->pid, thread_arg->context,
                                     thread_arg->context_size) == false;
}



bool ExceptionHandler::HandleSignal(int sig, siginfo_t* info, void* uc) {
  if (filter_ && !filter_(callback_context_))
    return false;

  
  sys_prctl(PR_SET_DUMPABLE, 1);

  CrashContext context;
  memcpy(&context.siginfo, info, sizeof(siginfo_t));
  memcpy(&context.context, uc, sizeof(struct ucontext));
  memcpy(&context.float_state, ((struct ucontext *)uc)->uc_mcontext.fpregs,
         sizeof(context.float_state));
  context.tid = sys_gettid();

  if (crash_handler_ && crash_handler_(&context, sizeof(context),
                                       callback_context_))
    return true;

  static const unsigned kChildStackSize = 8000;
  PageAllocator allocator;
  uint8_t* stack = (uint8_t*) allocator.Alloc(kChildStackSize);
  if (!stack)
    return false;
  
  stack += kChildStackSize;
  my_memset(stack - 16, 0, 16);

  ThreadArgument thread_arg;
  thread_arg.handler = this;
  thread_arg.pid = getpid();
  thread_arg.context = &context;
  thread_arg.context_size = sizeof(context);

  const pid_t child = sys_clone(
      ThreadEntry, stack, CLONE_FILES | CLONE_FS | CLONE_UNTRACED,
      &thread_arg, NULL, NULL, NULL);
  int r, status;
  do {
    r = sys_waitpid(child, &status, __WALL);
  } while (r == -1 && errno == EINTR);

  if (r == -1) {
    static const char msg[] = "ExceptionHandler::HandleSignal: waitpid failed:";
    sys_write(2, msg, sizeof(msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
  }

  bool success = r != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;

  if (callback_)
    success = callback_(dump_path_c_, next_minidump_id_c_,
                        callback_context_, success);

  return success;
}



bool ExceptionHandler::DoDump(pid_t crashing_process, const void* context,
                              size_t context_size) {
  return google_breakpad::WriteMinidump(
      next_minidump_path_c_, crashing_process, context, context_size);
}

}  
