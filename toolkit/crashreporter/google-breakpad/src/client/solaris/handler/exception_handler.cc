






























#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <ctime>

#include "client/solaris/handler/exception_handler.h"
#include "common/solaris/guid_creator.h"
#include "common/solaris/message_output.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {


static const int kSigTable[] = {
  SIGSEGV,
  SIGABRT,
  SIGFPE,
  SIGILL,
  SIGBUS
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

  if (install_handler) {
    SetupHandler();
  }

  if (install_handler) {
    pthread_mutex_lock(&handler_stack_mutex_);

    if (handler_stack_ == NULL)
      handler_stack_ = new std::vector<ExceptionHandler *>;
    handler_stack_->push_back(this);
    pthread_mutex_unlock(&handler_stack_mutex_);
  }
}

ExceptionHandler::~ExceptionHandler() {
  TeardownAllHandlers();
  pthread_mutex_lock(&handler_stack_mutex_);
  if (handler_stack_->back() == this) {
    handler_stack_->pop_back();
  } else {
    print_message1(2, "warning: removing Breakpad handler out of order\n");
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
  return InternalWriteMinidump(0, 0, NULL);
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
  for (size_t i = 0; i < sizeof(kSigTable) / sizeof(kSigTable[0]); ++i)
    SetupHandler(kSigTable[i]);
}

void ExceptionHandler::SetupHandler(int signo) {
  struct sigaction act, old_act;
  act.sa_handler = HandleException;
  act.sa_flags = SA_ONSTACK;
  if (sigaction(signo, &act, &old_act) < 0)
    return;
  old_handlers_[signo] = old_act.sa_handler;
}

void ExceptionHandler::TeardownHandler(int signo) {
  if (old_handlers_.find(signo) != old_handlers_.end()) {
    struct sigaction act;
    act.sa_handler = old_handlers_[signo];
    act.sa_flags = 0;
    sigaction(signo, &act, 0);
  }
}

void ExceptionHandler::TeardownAllHandlers() {
  for (size_t i = 0; i < sizeof(kSigTable) / sizeof(kSigTable[0]); ++i) {
    TeardownHandler(kSigTable[i]);
  }
}


void ExceptionHandler::HandleException(int signo) {

  
  
  
  
  
  
  
  
  
  
  
  uintptr_t current_ebp = (uintptr_t)_getfp();

  pthread_mutex_lock(&handler_stack_mutex_);
  ExceptionHandler *current_handler =
    handler_stack_->at(handler_stack_->size() - ++handler_stack_index_);
  pthread_mutex_unlock(&handler_stack_mutex_);

  
  current_handler->TeardownHandler(signo);

  ucontext_t *sig_ctx = NULL;
  if (current_handler->InternalWriteMinidump(signo, current_ebp, &sig_ctx)) {

    
    exit(EXIT_FAILURE);
  } else {
    
    
    typedef void (*SignalHandler)(int signo);
    SignalHandler old_handler =
      reinterpret_cast<SignalHandler>(current_handler->old_handlers_[signo]);
    if (old_handler != NULL)
      old_handler(signo);
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
                                             ucontext_t **sig_ctx) {
  if (filter_ && !filter_(callback_context_))
    return false;

  bool success = false;
  GUID guid;
  char guid_str[kGUIDStringLength + 1];
  if (CreateGUID(&guid) && GUIDToString(&guid, guid_str, sizeof(guid_str))) {
    char minidump_path[PATH_MAX];
    snprintf(minidump_path, sizeof(minidump_path), "%s/%s.dmp",
             dump_path_c_, guid_str);

    
    
    sigset_t sig_blocked, sig_old;
    bool blocked = true;
    sigfillset(&sig_blocked);
    for (size_t i = 0; i < sizeof(kSigTable) / sizeof(kSigTable[0]); ++i)
      sigdelset(&sig_blocked, kSigTable[i]);
    if (sigprocmask(SIG_BLOCK, &sig_blocked, &sig_old) != 0) {
      blocked = false;
      print_message1(2, "HandleException: failed to block signals.\n");
    }

    success = minidump_generator_.WriteMinidumpToFile(
                       minidump_path, signo, sighandler_ebp, sig_ctx);

    
    if (blocked)
      sigprocmask(SIG_SETMASK, &sig_old, &sig_old);

    if (callback_)
      success = callback_(dump_path_c_, guid_str, callback_context_, success);
  }
  return success;
}

}  
