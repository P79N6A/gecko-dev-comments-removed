






























#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <linux/limits.h>

#include "client/linux/handler/exception_handler.h"
#include "common/linux/guid_creator.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {


int SigTable[] = {
#if defined(SIGSEGV)
  SIGSEGV,
#endif
#ifdef SIGABRT
  SIGABRT,
#endif
#ifdef SIGFPE
  SIGFPE,
#endif
#ifdef SIGILL
  SIGILL,
#endif
#ifdef SIGBUS
  SIGBUS,
#endif
};

std::vector<ExceptionHandler*> *ExceptionHandler::handler_stack_ = NULL;
int ExceptionHandler::handler_stack_index_ = 0;
pthread_mutex_t ExceptionHandler::handler_stack_mutex_ =
PTHREAD_MUTEX_INITIALIZER;

ExceptionHandler::ExceptionHandler(const string &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   bool install_handler)
    : filter_(filter),
      callback_(callback),
      callback_context_(callback_context),
      dump_path_(),
      installed_handler_(install_handler) {
  set_dump_path(dump_path);

  act_.sa_handler = HandleException;
  act_.sa_flags = SA_ONSTACK;
  sigemptyset(&act_.sa_mask);
  
  
  for ( size_t i = 0; i < sizeof(SigTable) / sizeof(SigTable[0]); ++i) {
    sigaddset(&act_.sa_mask, SigTable[i]);
  }

  if (install_handler) {
    SetupHandler();
    pthread_mutex_lock(&handler_stack_mutex_);
    if (handler_stack_ == NULL)
      handler_stack_ = new std::vector<ExceptionHandler *>;
    handler_stack_->push_back(this);
    pthread_mutex_unlock(&handler_stack_mutex_);
  }
}

ExceptionHandler::~ExceptionHandler() {
  TeardownAllHandler();
  pthread_mutex_lock(&handler_stack_mutex_);
  if (handler_stack_->back() == this) {
    handler_stack_->pop_back();
  } else {
    fprintf(stderr, "warning: removing Breakpad handler out of order\n");
    for (std::vector<ExceptionHandler *>::iterator iterator =
         handler_stack_->begin();
         iterator != handler_stack_->end();
         ++iterator) {
      if (*iterator == this) {
        handler_stack_->erase(iterator);
      }
    }
  }

  if (handler_stack_->empty()) {
    
    
    delete handler_stack_;
    handler_stack_ = NULL;
  }
  pthread_mutex_unlock(&handler_stack_mutex_);
}

bool ExceptionHandler::WriteMinidump() {
  bool success = InternalWriteMinidump(0, 0, NULL);
  UpdateNextID();
  return success;
}


bool ExceptionHandler::WriteMinidump(const string &dump_path,
                   MinidumpCallback callback,
                   void *callback_context) {
  ExceptionHandler handler(dump_path, NULL, callback,
                           callback_context, false);
  return handler.InternalWriteMinidump(0, 0, NULL);
}

void ExceptionHandler::SetupHandler() {
  
  
  struct sigaltstack sig_stack;
  sig_stack.ss_sp = malloc(MINSIGSTKSZ);
  if (sig_stack.ss_sp == NULL)
    return;
  sig_stack.ss_size = MINSIGSTKSZ;
  sig_stack.ss_flags = 0;

  if (sigaltstack(&sig_stack, NULL) < 0)
    return;
  for (size_t i = 0; i < sizeof(SigTable) / sizeof(SigTable[0]); ++i)
    SetupHandler(SigTable[i]);
}

void ExceptionHandler::SetupHandler(int signo) {

  
  
  
  
  struct sigaction *old_act = &old_actions_[signo];

  if (sigaction(signo, &act_, old_act) < 0)
   return;
}

void ExceptionHandler::TeardownHandler(int signo) {
  TeardownHandler(signo, NULL);
}

void ExceptionHandler::TeardownHandler(int signo, struct sigaction *final_handler) {
  if (old_actions_[signo].sa_handler) {
    struct sigaction *act = &old_actions_[signo];
    sigaction(signo, act, final_handler);
    memset(&old_actions_[signo], 0x0, sizeof(struct sigaction));
  }
}

void ExceptionHandler::TeardownAllHandler() {
  for (size_t i = 0; i < sizeof(SigTable) / sizeof(SigTable[0]); ++i) {
    TeardownHandler(SigTable[i]);
  }
}


void ExceptionHandler::HandleException(int signo) {
  
  
  
  
  
  
  
  
  
  uintptr_t current_ebp = 0;
  asm volatile ("movl %%ebp, %0"
                :"=m"(current_ebp));

  pthread_mutex_lock(&handler_stack_mutex_);
  ExceptionHandler *current_handler =
    handler_stack_->at(handler_stack_->size() - ++handler_stack_index_);
  pthread_mutex_unlock(&handler_stack_mutex_);

  
  struct sigaction old_action;
  current_handler->TeardownHandler(signo, &old_action);

  struct sigcontext *sig_ctx = NULL;
  if (current_handler->InternalWriteMinidump(signo, current_ebp, &sig_ctx)) {
    
    exit(EXIT_FAILURE);
  } else {
    
    
    if (old_action.sa_handler != NULL && sig_ctx != NULL) {

      
      
      typedef void (*SignalHandler)(int signo, struct sigcontext);

      SignalHandler old_handler =
          reinterpret_cast<SignalHandler>(old_action.sa_handler);

      sigset_t old_set;
      
      
      sigprocmask(SIG_BLOCK, &old_action.sa_mask, &old_set);
      old_handler(signo, *sig_ctx);
      sigprocmask(SIG_SETMASK, &old_set, NULL);
    }

  }

  pthread_mutex_lock(&handler_stack_mutex_);
  current_handler->SetupHandler(signo);
  --handler_stack_index_;
  
  
  
  
  
  if (handler_stack_index_ == 0)
    signal(signo, SIG_DFL);
  pthread_mutex_unlock(&handler_stack_mutex_);
}

bool ExceptionHandler::InternalWriteMinidump(int signo,
                                             uintptr_t sighandler_ebp,
                                             struct sigcontext **sig_ctx) {
  if (filter_ && !filter_(callback_context_))
    return false;

  bool success = false;
  
  
  sigset_t sig_blocked, sig_old;
  bool blocked = true;
  sigfillset(&sig_blocked);
  for (size_t i = 0; i < sizeof(SigTable) / sizeof(SigTable[0]); ++i)
    sigdelset(&sig_blocked, SigTable[i]);
  if (sigprocmask(SIG_BLOCK, &sig_blocked, &sig_old) != 0) {
    blocked = false;
    fprintf(stderr, "google_breakpad::ExceptionHandler::HandleException: "
                    "failed to block signals.\n");
  }

  success = minidump_generator_.WriteMinidumpToFile(
                     next_minidump_path_c_, signo, sighandler_ebp, sig_ctx);

  
  if (blocked) {
    sigprocmask(SIG_SETMASK, &sig_old, NULL);
  }

  if (callback_)
    success = callback_(dump_path_c_, next_minidump_id_c_,
                          callback_context_, success);
  return success;
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

}  
