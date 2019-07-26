
































































#include "client/linux/handler/exception_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/signal.h>
#include <sys/ucontext.h>
#include <sys/user.h>
#include <ucontext.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "common/linux/linux_libc_support.h"
#include "common/memory.h"
#include "client/linux/log/log.h"
#include "client/linux/minidump_writer/linux_dumper.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "common/linux/eintr_wrapper.h"
#include "third_party/lss/linux_syscall_support.h"

#include "linux/sched.h"

#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif


static int tgkill(pid_t tgid, pid_t tid, int sig) {
  return syscall(__NR_tgkill, tgid, tid, sig);
  return 0;
}

namespace google_breakpad {

namespace {



const int kExceptionSignals[] = {
  SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
};
const int kNumHandledSignals =
    sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);
struct sigaction old_handlers[kNumHandledSignals];
bool handlers_installed = false;



stack_t old_stack;
stack_t new_stack;
bool stack_installed = false;




void InstallAlternateStackLocked() {
  if (stack_installed)
    return;

  memset(&old_stack, 0, sizeof(old_stack));
  memset(&new_stack, 0, sizeof(new_stack));

  
  
  
  static const unsigned kSigStackSize = std::max(8192, SIGSTKSZ);

  
  
  if (sys_sigaltstack(NULL, &old_stack) == -1 || !old_stack.ss_sp ||
      old_stack.ss_size < kSigStackSize) {
    new_stack.ss_sp = malloc(kSigStackSize);
    new_stack.ss_size = kSigStackSize;

    if (sys_sigaltstack(&new_stack, NULL) == -1) {
      free(new_stack.ss_sp);
      return;
    }
    stack_installed = true;
  }
}


void RestoreAlternateStackLocked() {
  if (!stack_installed)
    return;

  stack_t current_stack;
  if (sys_sigaltstack(NULL, &current_stack) == -1)
    return;

  
  
  if (current_stack.ss_sp == new_stack.ss_sp) {
    if (old_stack.ss_sp) {
      if (sys_sigaltstack(&old_stack, NULL) == -1)
        return;
    } else {
      stack_t disable_stack;
      disable_stack.ss_flags = SS_DISABLE;
      if (sys_sigaltstack(&disable_stack, NULL) == -1)
        return;
    }
  }

  free(new_stack.ss_sp);
  stack_installed = false;
}

}  



std::vector<ExceptionHandler*>* ExceptionHandler::handler_stack_ = NULL;
pthread_mutex_t ExceptionHandler::handler_stack_mutex_ =
    PTHREAD_MUTEX_INITIALIZER;


ExceptionHandler::ExceptionHandler(const MinidumpDescriptor& descriptor,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void* callback_context,
                                   bool install_handler,
                                   const int server_fd)
    : filter_(filter),
      callback_(callback),
      callback_context_(callback_context),
      minidump_descriptor_(descriptor),
      crash_handler_(NULL) {
  if (server_fd >= 0)
    crash_generation_client_.reset(CrashGenerationClient::TryCreate(server_fd));

  if (!IsOutOfProcess() && !minidump_descriptor_.IsFD())
    minidump_descriptor_.UpdatePath();

  pthread_mutex_lock(&handler_stack_mutex_);
  if (!handler_stack_)
    handler_stack_ = new std::vector<ExceptionHandler*>;
  if (install_handler) {
    InstallAlternateStackLocked();
    InstallHandlersLocked();
  }
  handler_stack_->push_back(this);
  pthread_mutex_unlock(&handler_stack_mutex_);
}


ExceptionHandler::~ExceptionHandler() {
  pthread_mutex_lock(&handler_stack_mutex_);
  std::vector<ExceptionHandler*>::iterator handler =
      std::find(handler_stack_->begin(), handler_stack_->end(), this);
  handler_stack_->erase(handler);
  if (handler_stack_->empty()) {
    RestoreAlternateStackLocked();
    RestoreHandlersLocked();
  }
  pthread_mutex_unlock(&handler_stack_mutex_);
}



bool ExceptionHandler::InstallHandlersLocked() {
  if (handlers_installed)
    return false;

  
  for (int i = 0; i < kNumHandledSignals; ++i) {
    if (sigaction(kExceptionSignals[i], NULL, &old_handlers[i]) == -1)
      return false;
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);

  
  for (int i = 0; i < kNumHandledSignals; ++i)
    sigaddset(&sa.sa_mask, kExceptionSignals[i]);

  sa.sa_sigaction = SignalHandler;
  sa.sa_flags = SA_ONSTACK | SA_SIGINFO;

  for (int i = 0; i < kNumHandledSignals; ++i) {
    if (sigaction(kExceptionSignals[i], &sa, NULL) == -1) {
      
      
    }
  }
  handlers_installed = true;
  return true;
}




void ExceptionHandler::RestoreHandlersLocked() {
  if (!handlers_installed)
    return;

  for (int i = 0; i < kNumHandledSignals; ++i) {
    if (sigaction(kExceptionSignals[i], &old_handlers[i], NULL) == -1) {
      signal(kExceptionSignals[i], SIG_DFL);
    }
  }
  handlers_installed = false;
}








void ExceptionHandler::SignalHandler(int sig, siginfo_t* info, void* uc) {
  
  pthread_mutex_lock(&handler_stack_mutex_);

  
  
  
  
  
  
  
  
  
  
  struct sigaction cur_handler;
  if (sigaction(sig, NULL, &cur_handler) == 0 &&
      (cur_handler.sa_flags & SA_SIGINFO) == 0) {
    
    sigemptyset(&cur_handler.sa_mask);
    sigaddset(&cur_handler.sa_mask, sig);

    cur_handler.sa_sigaction = SignalHandler;
    cur_handler.sa_flags = SA_ONSTACK | SA_SIGINFO;

    if (sigaction(sig, &cur_handler, NULL) == -1) {
      
      
      signal(sig, SIG_DFL);
    }
    pthread_mutex_unlock(&handler_stack_mutex_);
    return;
  }

  bool handled = false;
  for (int i = handler_stack_->size() - 1; !handled && i >= 0; --i) {
    handled = (*handler_stack_)[i]->HandleSignal(sig, info, uc);
  }

  
  
  
  
  
  if (handled) {
    signal(sig, SIG_DFL);
  } else {
    RestoreHandlersLocked();
  }

  pthread_mutex_unlock(&handler_stack_mutex_);

  if (info->si_code <= 0) {
    
    
    
    if (tgkill(getpid(), syscall(__NR_gettid), sig) < 0) {
      
      
      
      _exit(1);
    }
  } else {
    
    
    
  }
}

struct ThreadArgument {
  pid_t pid;  
  const MinidumpDescriptor* minidump_descriptor;
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

  
  bool signal_trusted = info->si_code > 0;
  bool signal_pid_trusted = info->si_code == SI_USER ||
      info->si_code == SI_TKILL;
  if (signal_trusted || (signal_pid_trusted && info->si_pid == getpid())) {
    sys_prctl(PR_SET_DUMPABLE, 1);
  }
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
    if (crash_handler_(&context, sizeof(context), callback_context_)) {
      return true;
    }
  }
  return GenerateDump(&context);
}



bool ExceptionHandler::SimulateSignalDelivery(int sig) {
  siginfo_t siginfo = {};
  
  
  siginfo.si_code = SI_USER;
  siginfo.si_pid = getpid();
  struct ucontext context;
  getcontext(&context);
  return HandleSignal(sig, &siginfo, &context);
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
  thread_arg.minidump_descriptor = &minidump_descriptor_;
  thread_arg.pid = getpid();
  thread_arg.context = context;
  thread_arg.context_size = sizeof(*context);

  
  
  
  
  if(sys_pipe(fdes) == -1) {
    
    
    
    static const char no_pipe_msg[] = "ExceptionHandler::GenerateDump \
                                       sys_pipe failed:";
    logger::write(no_pipe_msg, sizeof(no_pipe_msg) - 1);
    logger::write(strerror(errno), strlen(strerror(errno)));
    logger::write("\n", 1);
  }

  const pid_t child = sys_clone(
      ThreadEntry, stack, CLONE_FILES | CLONE_FS | CLONE_UNTRACED,
      &thread_arg, NULL, NULL, NULL);

  int r, status;
  
  sys_prctl(PR_SET_PTRACER, child);
  SendContinueSignalToChild();
  do {
    r = sys_waitpid(child, &status, __WALL);
  } while (r == -1 && errno == EINTR);

  sys_close(fdes[0]);
  sys_close(fdes[1]);

  if (r == -1) {
    static const char msg[] = "ExceptionHandler::GenerateDump waitpid failed:";
    logger::write(msg, sizeof(msg) - 1);
    logger::write(strerror(errno), strlen(strerror(errno)));
    logger::write("\n", 1);
  }

  bool success = r != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
  if (callback_)
    success = callback_(minidump_descriptor_, callback_context_, success);
  return success;
}


void ExceptionHandler::SendContinueSignalToChild() {
  static const char okToContinueMessage = 'a';
  int r;
  r = HANDLE_EINTR(sys_write(fdes[1], &okToContinueMessage, sizeof(char)));
  if(r == -1) {
    static const char msg[] = "ExceptionHandler::SendContinueSignalToChild \
                               sys_write failed:";
    logger::write(msg, sizeof(msg) - 1);
    logger::write(strerror(errno), strlen(strerror(errno)));
    logger::write("\n", 1);
  }
}



void ExceptionHandler::WaitForContinueSignal() {
  int r;
  char receivedMessage;
  r = HANDLE_EINTR(sys_read(fdes[0], &receivedMessage, sizeof(char)));
  if(r == -1) {
    static const char msg[] = "ExceptionHandler::WaitForContinueSignal \
                               sys_read failed:";
    logger::write(msg, sizeof(msg) - 1);
    logger::write(strerror(errno), strlen(strerror(errno)));
    logger::write("\n", 1);
  }
}



bool ExceptionHandler::DoDump(pid_t crashing_process, const void* context,
                              size_t context_size) {
  if (minidump_descriptor_.IsFD()) {
    return google_breakpad::WriteMinidump(minidump_descriptor_.fd(),
                                          minidump_descriptor_.size_limit(),
                                          crashing_process,
                                          context,
                                          context_size,
                                          mapping_list_,
                                          app_memory_list_);
  }
  return google_breakpad::WriteMinidump(minidump_descriptor_.path(),
                                        minidump_descriptor_.size_limit(),
                                        crashing_process,
                                        context,
                                        context_size,
                                        mapping_list_,
                                        app_memory_list_);
}


bool ExceptionHandler::WriteMinidump(const string& dump_path,
                                     MinidumpCallback callback,
                                     void* callback_context) {
  MinidumpDescriptor descriptor(dump_path);
  ExceptionHandler eh(descriptor, NULL, callback, callback_context, false, -1);
  return eh.WriteMinidump();
}

bool ExceptionHandler::WriteMinidump() {
  if (!IsOutOfProcess() && !minidump_descriptor_.IsFD()) {
    
    
    
    
    minidump_descriptor_.UpdatePath();
  } else if (minidump_descriptor_.IsFD()) {
    
    
    lseek(minidump_descriptor_.fd(), 0, SEEK_SET);
    static_cast<void>(ftruncate(minidump_descriptor_.fd(), 0));
  }

  
  sys_prctl(PR_SET_DUMPABLE, 1);

  CrashContext context;
  int getcontext_result = getcontext(&context.context);
  if (getcontext_result)
    return false;
#if !defined(__ARM_EABI__)
  
  memcpy(&context.float_state, context.context.uc_mcontext.fpregs,
         sizeof(context.float_state));
#endif
  context.tid = sys_gettid();

  
  memset(&context.siginfo, 0, sizeof(context.siginfo));
  context.siginfo.si_signo = MD_EXCEPTION_CODE_LIN_DUMP_REQUESTED;
#if defined(__i386__)
  context.siginfo.si_addr =
      reinterpret_cast<void*>(context.context.uc_mcontext.gregs[REG_EIP]);
#elif defined(__x86_64__)
  context.siginfo.si_addr =
      reinterpret_cast<void*>(context.context.uc_mcontext.gregs[REG_RIP]);
#elif defined(__arm__)
  context.siginfo.si_addr =
      reinterpret_cast<void*>(context.context.uc_mcontext.arm_pc);
#else
#error "This code has not been ported to your platform yet."
#endif

  return GenerateDump(&context);
}

void ExceptionHandler::AddMappingInfo(const string& name,
                                      const uint8_t identifier[sizeof(MDGUID)],
                                      uintptr_t start_address,
                                      size_t mapping_size,
                                      size_t file_offset) {
  MappingInfo info;
  info.start_addr = start_address;
  info.size = mapping_size;
  info.offset = file_offset;
  strncpy(info.name, name.c_str(), sizeof(info.name) - 1);
  info.name[sizeof(info.name) - 1] = '\0';

  MappingEntry mapping;
  mapping.first = info;
  memcpy(mapping.second, identifier, sizeof(MDGUID));
  mapping_list_.push_back(mapping);
}

void ExceptionHandler::RegisterAppMemory(void* ptr, size_t length) {
  AppMemoryList::iterator iter =
    std::find(app_memory_list_.begin(), app_memory_list_.end(), ptr);
  if (iter != app_memory_list_.end()) {
    
    return;
  }

  AppMemory app_memory;
  app_memory.ptr = ptr;
  app_memory.length = length;
  app_memory_list_.push_back(app_memory);
}

void ExceptionHandler::UnregisterAppMemory(void* ptr) {
  AppMemoryList::iterator iter =
    std::find(app_memory_list_.begin(), app_memory_list_.end(), ptr);
  if (iter != app_memory_list_.end()) {
    app_memory_list_.erase(iter);
  }
}


bool ExceptionHandler::WriteMinidumpForChild(pid_t child,
                                             pid_t child_blamed_thread,
                                             const string& dump_path,
                                             MinidumpCallback callback,
                                             void* callback_context) {
  
  MinidumpDescriptor descriptor(dump_path);
  descriptor.UpdatePath();
  if (!google_breakpad::WriteMinidump(descriptor.path(),
                                      child,
                                      child_blamed_thread))
      return false;

  return callback ? callback(descriptor, callback_context, true) : true;
}

}  
