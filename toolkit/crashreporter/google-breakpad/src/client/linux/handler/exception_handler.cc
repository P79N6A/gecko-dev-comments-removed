
































































#include "client/linux/handler/exception_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#if !defined(__ANDROID__)
#include <sys/signal.h>
#endif
#include <sys/syscall.h>
#if !defined(__ANDROID__)
#include <sys/ucontext.h>
#include <sys/user.h>
#endif
#include <sys/wait.h>
#if !defined(__ANDROID__)
#include <ucontext.h>
#endif
#include <unistd.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "common/linux/linux_libc_support.h"
#include "common/linux/linux_syscall_support.h"
#include "common/memory.h"
#include "client/linux/minidump_writer/linux_dumper.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "common/linux/guid_creator.h"
#include "common/linux/eintr_wrapper.h"

#include "linux/sched.h"

#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif


static int tgkill(pid_t tgid, pid_t tid, int sig) {
  return syscall(__NR_tgkill, tgid, tid, sig);
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
    handler_installed_(install_handler)
{
  Init(dump_path, -1);
}

ExceptionHandler::ExceptionHandler(const std::string &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void* callback_context,
                                   bool install_handler,
                                   const int server_fd)
  : filter_(filter),
    callback_(callback),
    callback_context_(callback_context),
    handler_installed_(install_handler)
{
  Init(dump_path, server_fd);
}


ExceptionHandler::~ExceptionHandler() {
  UninstallHandlers();
}

void ExceptionHandler::Init(const std::string &dump_path,
                            const int server_fd)
{
  crash_handler_ = NULL;
  if (0 <= server_fd)
    crash_generation_client_
      .reset(CrashGenerationClient::TryCreate(server_fd));

  if (handler_installed_)
    InstallHandlers();

  if (!IsOutOfProcess())
    set_dump_path(dump_path);

  pthread_mutex_lock(&handler_stack_mutex_);
  if (handler_stack_ == NULL)
    handler_stack_ = new std::vector<ExceptionHandler *>;
  handler_stack_->push_back(this);
  pthread_mutex_unlock(&handler_stack_mutex_);
}


bool ExceptionHandler::InstallHandlers() {
  
  

  
  
  static const unsigned kSigStackSize = 8192;

  signal_stack = malloc(kSigStackSize);
  stack_t stack;
  memset(&stack, 0, sizeof(stack));
  stack.ss_sp = signal_stack;
  stack.ss_size = kSigStackSize;

  if (sys_sigaltstack(&stack, NULL) == -1)
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
  pthread_mutex_lock(&handler_stack_mutex_);
  std::vector<ExceptionHandler*>::iterator handler =
      std::find(handler_stack_->begin(), handler_stack_->end(), this);
  handler_stack_->erase(handler);
  pthread_mutex_unlock(&handler_stack_mutex_);
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

  
  tgkill(getpid(), syscall(__NR_gettid), sig);
  _exit(1);

  
}

struct ThreadArgument {
  pid_t pid;  
  ExceptionHandler* handler;
  const void* context;  
  size_t context_size;
};




int ExceptionHandler::ThreadEntry(void *arg) {
  const ThreadArgument *thread_arg = reinterpret_cast<ThreadArgument*>(arg);

  
  
  thread_arg->handler->WaitForContinueSignal();

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
#if !defined(__ARM_EABI__)
  
  struct ucontext *uc_ptr = (struct ucontext*)uc;
  if (uc_ptr->uc_mcontext.fpregs) {
    memcpy(&context.float_state,
           uc_ptr->uc_mcontext.fpregs,
           sizeof(context.float_state));
  }
#endif
  context.tid = syscall(__NR_gettid);
  if (crash_handler_ != NULL) {
    if (crash_handler_(&context, sizeof(context),
                       callback_context_)) {
      return true;
    }
  }
  return GenerateDump(&context);
}


bool ExceptionHandler::GenerateDump(CrashContext *context) {
  if (IsOutOfProcess())
    return crash_generation_client_->RequestDump(context, sizeof(*context));

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
  thread_arg.context = context;
  thread_arg.context_size = sizeof(*context);

  
  
  
  
  if(sys_pipe(fdes) == -1) {
    
    
    
    static const char no_pipe_msg[] = "ExceptionHandler::GenerateDump \
                                       sys_pipe failed:";
    sys_write(2, no_pipe_msg, sizeof(no_pipe_msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
  }

  const pid_t child = sys_clone(
      ThreadEntry, stack, CLONE_FILES | CLONE_FS | CLONE_UNTRACED,
      &thread_arg, NULL, NULL, NULL);
  int r, status;
  
  prctl(PR_SET_PTRACER, child, 0, 0, 0);
  SendContinueSignalToChild();
  do {
    r = sys_waitpid(child, &status, __WALL);
  } while (r == -1 && errno == EINTR);

  sys_close(fdes[0]);
  sys_close(fdes[1]);

  if (r == -1) {
    static const char msg[] = "ExceptionHandler::GenerateDump waitpid failed:";
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


void ExceptionHandler::SendContinueSignalToChild() {
  static const char okToContinueMessage = 'a';
  int r;
  r = HANDLE_EINTR(sys_write(fdes[1], &okToContinueMessage, sizeof(char)));
  if(r == -1) {
    static const char msg[] = "ExceptionHandler::SendContinueSignalToChild \
                               sys_write failed:";
    sys_write(2, msg, sizeof(msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
  }
}



void ExceptionHandler::WaitForContinueSignal() {
  int r;
  char receivedMessage;
  r = HANDLE_EINTR(sys_read(fdes[0], &receivedMessage, sizeof(char)));
  if(r == -1) {
    static const char msg[] = "ExceptionHandler::WaitForContinueSignal \
                               sys_read failed:";
    sys_write(2, msg, sizeof(msg) - 1);
    sys_write(2, strerror(errno), strlen(strerror(errno)));
    sys_write(2, "\n", 1);
  }
}



bool ExceptionHandler::DoDump(pid_t crashing_process, const void* context,
                              size_t context_size) {
  return google_breakpad::WriteMinidump(next_minidump_path_c_,
                                        crashing_process,
                                        context,
                                        context_size,
                                        mapping_info_);
}


bool ExceptionHandler::WriteMinidump(const std::string &dump_path,
                                     MinidumpCallback callback,
                                     void* callback_context) {
  return WriteMinidump(dump_path, false, callback, callback_context);
}


bool ExceptionHandler::WriteMinidump(const std::string &dump_path,
                                     bool write_exception_stream,
                                     MinidumpCallback callback,
                                     void* callback_context) {
  ExceptionHandler eh(dump_path, NULL, callback, callback_context, false);
  return eh.WriteMinidump(write_exception_stream);
}

bool ExceptionHandler::WriteMinidump() {
  return WriteMinidump(false);
}

bool ExceptionHandler::WriteMinidump(bool write_exception_stream) {
#if !defined(__ARM_EABI__)
  
  sys_prctl(PR_SET_DUMPABLE, 1);

  CrashContext context;
  int getcontext_result = getcontext(&context.context);
  if (getcontext_result)
    return false;
  memcpy(&context.float_state, context.context.uc_mcontext.fpregs,
         sizeof(context.float_state));
  context.tid = sys_gettid();

  if (write_exception_stream) {
    memset(&context.siginfo, 0, sizeof(context.siginfo));
    context.siginfo.si_signo = SIGSTOP;
#if defined(__i386)
    context.siginfo.si_addr =
      reinterpret_cast<void*>(context.context.uc_mcontext.gregs[REG_EIP]);
#elif defined(__x86_64)
    context.siginfo.si_addr =
      reinterpret_cast<void*>(context.context.uc_mcontext.gregs[REG_RIP]);
#elif defined(__ARMEL__)
    context.siginfo.si_addr =
      reinterpret_cast<void*>(context.context.uc_mcontext.arm_ip);
#endif
  }

  bool success = GenerateDump(&context);
  UpdateNextID();
  return success;
#else
  return false;
#endif  
}


bool ExceptionHandler::WriteMinidumpForChild(pid_t child,
                                             pid_t child_blamed_thread,
                                             const std::string &dump_path,
                                             MinidumpCallback callback,
                                             void *callback_context)
{
  
  ExceptionHandler eh(dump_path, NULL, NULL, NULL, false);
  if (!google_breakpad::WriteMinidump(eh.next_minidump_path_c_,
                                      child,
                                      child_blamed_thread))
      return false;

  return callback ? callback(eh.dump_path_c_, eh.next_minidump_id_c_,
                             callback_context, true) : true;
}

void ExceptionHandler::AddMappingInfo(const std::string& name,
                                      const u_int8_t identifier[sizeof(MDGUID)],
                                      uintptr_t start_address,
                                      size_t mapping_size,
                                      size_t file_offset) {
   MappingInfo info;
   info.start_addr = start_address;
   info.size = mapping_size;
   info.offset = file_offset;
   strncpy(info.name, name.c_str(), std::min(name.size() + 1, sizeof(info)));
 
   std::pair<MappingInfo, u_int8_t[sizeof(MDGUID)]> mapping;
   mapping.first = info;
   memcpy(mapping.second, identifier, sizeof(MDGUID));
   mapping_info_.push_back(mapping);
}

}  
