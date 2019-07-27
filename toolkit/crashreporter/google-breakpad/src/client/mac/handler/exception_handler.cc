




























#include <mach/exc.h>
#include <mach/mig.h>
#include <pthread.h>
#include <signal.h>
#include <TargetConditionals.h>

#include <map>

#include "client/mac/handler/exception_handler.h"
#include "client/mac/handler/minidump_generator.h"
#include "common/mac/macho_utilities.h"
#include "common/mac/scoped_task_suspend-inl.h"
#include "google_breakpad/common/minidump_exception_mac.h"

#ifndef __EXCEPTIONS





#undef try
#define try       if (true)
#undef catch
#define catch(X)  if (false)
#endif  

#ifndef USE_PROTECTED_ALLOCATIONS
#if TARGET_OS_IPHONE
#define USE_PROTECTED_ALLOCATIONS 1
#else
#define USE_PROTECTED_ALLOCATIONS 0
#endif
#endif





#if USE_PROTECTED_ALLOCATIONS
  #include "protected_memory_allocator.h"
  extern ProtectedMemoryAllocator *gBreakpadAllocator;
#endif

namespace google_breakpad {

static union {
#if USE_PROTECTED_ALLOCATIONS
  char protected_buffer[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
#endif
  google_breakpad::ExceptionHandler *handler;
} gProtectedData;

using std::map;



struct ExceptionMessage {
  mach_msg_header_t           header;
  mach_msg_body_t             body;
  mach_msg_port_descriptor_t  thread;
  mach_msg_port_descriptor_t  task;
  NDR_record_t                ndr;
  exception_type_t            exception;
  mach_msg_type_number_t      code_count;
  integer_t                   code[EXCEPTION_CODE_MAX];
  char                        padding[512];
};

struct ExceptionParameters {
  ExceptionParameters() : count(0) {}
  mach_msg_type_number_t count;
  exception_mask_t masks[EXC_TYPES_COUNT];
  mach_port_t ports[EXC_TYPES_COUNT];
  exception_behavior_t behaviors[EXC_TYPES_COUNT];
  thread_state_flavor_t flavors[EXC_TYPES_COUNT];
};

struct ExceptionReplyMessage {
  mach_msg_header_t  header;
  NDR_record_t       ndr;
  kern_return_t      return_code;
};



exception_mask_t s_exception_mask = EXC_MASK_BAD_ACCESS |
EXC_MASK_BAD_INSTRUCTION | EXC_MASK_ARITHMETIC | EXC_MASK_BREAKPOINT;

#if !TARGET_OS_IPHONE
extern "C" {
  
  boolean_t exc_server(mach_msg_header_t* request,
                       mach_msg_header_t* reply);

  
  
  kern_return_t catch_exception_raise(mach_port_t target_port,
                                      mach_port_t failed_thread,
                                      mach_port_t task,
                                      exception_type_t exception,
                                      exception_data_t code,
                                      mach_msg_type_number_t code_count)
      __attribute__((visibility("default")));
}
#endif

kern_return_t ForwardException(mach_port_t task,
                               mach_port_t failed_thread,
                               exception_type_t exception,
                               exception_data_t code,
                               mach_msg_type_number_t code_count);

#if TARGET_OS_IPHONE

boolean_t breakpad_exc_server(mach_msg_header_t* InHeadP,
                              mach_msg_header_t* OutHeadP) {
  OutHeadP->msgh_bits =
      MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(InHeadP->msgh_bits), 0);
  OutHeadP->msgh_remote_port = InHeadP->msgh_remote_port;
  
  OutHeadP->msgh_size = (mach_msg_size_t)sizeof(mig_reply_error_t);
  OutHeadP->msgh_local_port = MACH_PORT_NULL;
  OutHeadP->msgh_id = InHeadP->msgh_id + 100;

  if (InHeadP->msgh_id != 2401) {
    ((mig_reply_error_t*)OutHeadP)->NDR = NDR_record;
    ((mig_reply_error_t*)OutHeadP)->RetCode = MIG_BAD_ID;
    return FALSE;
  }

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
  typedef struct {
    mach_msg_header_t Head;
    
    mach_msg_body_t msgh_body;
    mach_msg_port_descriptor_t thread;
    mach_msg_port_descriptor_t task;
    
    NDR_record_t NDR;
    exception_type_t exception;
    mach_msg_type_number_t codeCnt;
    integer_t code[2];
    mach_msg_trailer_t trailer;
  } Request;

  typedef struct {
    mach_msg_header_t Head;
    NDR_record_t NDR;
    kern_return_t RetCode;
  } Reply;
#ifdef  __MigPackStructs
#pragma pack()
#endif

  Request* In0P = (Request*)InHeadP;
  Reply* OutP = (Reply*)OutHeadP;

  if (In0P->task.name != mach_task_self()) {
    return FALSE;
  }
  OutP->RetCode = ForwardException(In0P->task.name,
                                   In0P->thread.name,
                                   In0P->exception,
                                   In0P->code,
                                   In0P->codeCnt);
  OutP->NDR = NDR_record;
  return TRUE;
}
#else
boolean_t breakpad_exc_server(mach_msg_header_t* request,
                              mach_msg_header_t* reply) {
  return exc_server(request, reply);
}


kern_return_t catch_exception_raise(mach_port_t port, mach_port_t failed_thread,
                                    mach_port_t task,
                                    exception_type_t exception,
                                    exception_data_t code,
                                    mach_msg_type_number_t code_count) {
  if (task != mach_task_self()) {
    return KERN_FAILURE;
  }
  return ForwardException(task, failed_thread, exception, code, code_count);
}
#endif

ExceptionHandler::ExceptionHandler(const string &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void* callback_context,
                                   bool install_handler,
                                   const char* port_name)
    : dump_path_(),
      filter_(filter),
      callback_(callback),
      callback_context_(callback_context),
      directCallback_(NULL),
      handler_thread_(NULL),
      handler_port_(MACH_PORT_NULL),
      previous_(NULL),
      installed_exception_handler_(false),
      is_in_teardown_(false),
      last_minidump_write_result_(false),
      use_minidump_write_mutex_(false) {
  
  set_dump_path(dump_path);
  MinidumpGenerator::GatherSystemInformation();
#if !TARGET_OS_IPHONE
  if (port_name)
    crash_generation_client_.reset(new CrashGenerationClient(port_name));
#endif
  Setup(install_handler);
}



ExceptionHandler::ExceptionHandler(DirectCallback callback,
                                   void* callback_context,
                                   bool install_handler)
    : dump_path_(),
      filter_(NULL),
      callback_(NULL),
      callback_context_(callback_context),
      directCallback_(callback),
      handler_thread_(NULL),
      handler_port_(MACH_PORT_NULL),
      previous_(NULL),
      installed_exception_handler_(false),
      is_in_teardown_(false),
      last_minidump_write_result_(false),
      use_minidump_write_mutex_(false) {
  MinidumpGenerator::GatherSystemInformation();
  Setup(install_handler);
}

ExceptionHandler::~ExceptionHandler() {
  Teardown();
}

bool ExceptionHandler::WriteMinidump(bool write_exception_stream) {
  
  if (use_minidump_write_mutex_)
    return false;

  use_minidump_write_mutex_ = true;
  last_minidump_write_result_ = false;

  
  if (pthread_mutex_lock(&minidump_write_mutex_) == 0) {
    
    
    bool result = SendMessageToHandlerThread(write_exception_stream ?
                                             kWriteDumpWithExceptionMessage :
                                             kWriteDumpMessage);
    if (!result) {
      pthread_mutex_unlock(&minidump_write_mutex_);
      return false;
    }

    
    
    pthread_mutex_lock(&minidump_write_mutex_);
  }

  use_minidump_write_mutex_ = false;
  UpdateNextID();
  return last_minidump_write_result_;
}


bool ExceptionHandler::WriteMinidump(const string &dump_path,
                                     bool write_exception_stream,
                                     MinidumpCallback callback,
                                     void* callback_context) {
  ExceptionHandler handler(dump_path, NULL, callback, callback_context, false,
                           NULL);
  return handler.WriteMinidump(write_exception_stream);
}


bool ExceptionHandler::WriteMinidumpForChild(mach_port_t child,
                                             mach_port_t child_blamed_thread,
                                             const string &dump_path,
                                             MinidumpCallback callback,
                                             void* callback_context) {
  ScopedTaskSuspend suspend(child);

  MinidumpGenerator generator(child, MACH_PORT_NULL);
  string dump_id;
  string dump_filename = generator.UniqueNameInDirectory(dump_path, &dump_id);

  generator.SetExceptionInformation(EXC_BREAKPOINT,
#if defined(__i386__) || defined(__x86_64__)
                                    EXC_I386_BPT,
#elif defined(__ppc__) || defined(__ppc64__)
                                    EXC_PPC_BREAKPOINT,
#elif defined(__arm__)
                                    EXC_ARM_BREAKPOINT,
#else
#error architecture not supported
#endif
                                    0,
                                    child_blamed_thread);
  bool result = generator.Write(dump_filename.c_str());

  if (callback) {
    return callback(dump_path.c_str(), dump_id.c_str(),
                    callback_context, result);
  }
  return result;
}

bool ExceptionHandler::WriteMinidumpWithException(int exception_type,
                                                  int exception_code,
                                                  int exception_subcode,
                                                  ucontext_t* task_context,
                                                  mach_port_t thread_name,
                                                  bool exit_after_write,
                                                  bool report_current_thread) {
  bool result = false;

  if (directCallback_) {
    if (directCallback_(callback_context_,
                        exception_type,
                        exception_code,
                        exception_subcode,
                        thread_name) ) {
      if (exit_after_write)
        _exit(exception_type);
    }
#if !TARGET_OS_IPHONE
  } else if (IsOutOfProcess()) {
    if (exception_type && exception_code) {
      
      
      if (filter_ && !filter_(callback_context_))
        return false;
      result = crash_generation_client_->RequestDumpForException(
          exception_type,
          exception_code,
          exception_subcode,
          thread_name);
      if (result && exit_after_write) {
        _exit(exception_type);
      }
    }
#endif
  } else {
    string minidump_id;

    
    
    if (!dump_path_.empty()) {
      MinidumpGenerator md(mach_task_self(),
                           report_current_thread ? MACH_PORT_NULL :
                                                   mach_thread_self());
      md.SetTaskContext(task_context);
      if (exception_type && exception_code) {
        
        
        if (filter_ && !filter_(callback_context_))
          return false;

        md.SetExceptionInformation(exception_type, exception_code,
                                   exception_subcode, thread_name);
      }

      result = md.Write(next_minidump_path_c_);
    }

    
    if (callback_) {
      
      
      
      if (callback_(dump_path_c_, next_minidump_id_c_, callback_context_,
                    result)) {
        if (exit_after_write)
          _exit(exception_type);
      }
    }
  }

  return result;
}

kern_return_t ForwardException(mach_port_t task, mach_port_t failed_thread,
                               exception_type_t exception,
                               exception_data_t code,
                               mach_msg_type_number_t code_count) {
  
  
  
  ExceptionParameters current;

  current.count = EXC_TYPES_COUNT;
  mach_port_t current_task = mach_task_self();
  task_get_exception_ports(current_task,
                           s_exception_mask,
                           current.masks,
                           &current.count,
                           current.ports,
                           current.behaviors,
                           current.flavors);

  
  unsigned int found;
  for (found = 0; found < current.count; ++found) {
    if (current.masks[found] & (1 << exception)) {
      break;
    }
  }

  
  if (found == current.count) {
    fprintf(stderr, "** No previous ports for forwarding!! \n");
    exit(KERN_FAILURE);
  }

  mach_port_t target_port = current.ports[found];
  exception_behavior_t target_behavior = current.behaviors[found];

  kern_return_t result;
  switch (target_behavior) {
    case EXCEPTION_DEFAULT:
      result = exception_raise(target_port, failed_thread, task, exception,
                               code, code_count);
      break;

    default:
      fprintf(stderr, "** Unknown exception behavior: %d\n", target_behavior);
      result = KERN_FAILURE;
      break;
  }

  return result;
}


void* ExceptionHandler::WaitForMessage(void* exception_handler_class) {
  ExceptionHandler* self =
    reinterpret_cast<ExceptionHandler*>(exception_handler_class);
  ExceptionMessage receive;

  
  while (1) {
    receive.header.msgh_local_port = self->handler_port_;
    receive.header.msgh_size = static_cast<mach_msg_size_t>(sizeof(receive));
    kern_return_t result = mach_msg(&(receive.header),
                                    MACH_RCV_MSG | MACH_RCV_LARGE, 0,
                                    receive.header.msgh_size,
                                    self->handler_port_,
                                    MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);


    if (result == KERN_SUCCESS) {
      
      
      

      
      
      
      
      
      
      
      if (!receive.exception) {
        
        
        if (receive.header.msgh_id == kShutdownMessage)
          return NULL;

        self->SuspendThreads();

#if USE_PROTECTED_ALLOCATIONS
        if (gBreakpadAllocator)
          gBreakpadAllocator->Unprotect();
#endif

        mach_port_t thread = MACH_PORT_NULL;
        int exception_type = 0;
        int exception_code = 0;
        if (receive.header.msgh_id == kWriteDumpWithExceptionMessage) {
          thread = receive.thread.name;
          exception_type = EXC_BREAKPOINT;
#if defined(__i386__) || defined(__x86_64__)
          exception_code = EXC_I386_BPT;
#elif defined(__ppc__) || defined(__ppc64__)
          exception_code = EXC_PPC_BREAKPOINT;
#elif defined(__arm__)
          exception_code = EXC_ARM_BREAKPOINT;
#else
#error architecture not supported
#endif
        }

        
        self->last_minidump_write_result_ =
          self->WriteMinidumpWithException(exception_type, exception_code,
                                           0, NULL, thread,
                                           false, false);

#if USE_PROTECTED_ALLOCATIONS
        if (gBreakpadAllocator)
          gBreakpadAllocator->Protect();
#endif

        self->ResumeThreads();

        if (self->use_minidump_write_mutex_)
          pthread_mutex_unlock(&self->minidump_write_mutex_);
      } else {
        
        
        
        
        
        
        
        
        if (receive.task.name == mach_task_self()) {
          self->SuspendThreads();

#if USE_PROTECTED_ALLOCATIONS
        if (gBreakpadAllocator)
          gBreakpadAllocator->Unprotect();
#endif

        int subcode = 0;
        if (receive.exception == EXC_BAD_ACCESS && receive.code_count > 1)
          subcode = receive.code[1];

        
        self->WriteMinidumpWithException(receive.exception, receive.code[0],
                                         subcode, NULL, receive.thread.name,
                                         true, false);

#if USE_PROTECTED_ALLOCATIONS
        
        
        
        if (gBreakpadAllocator)
          gBreakpadAllocator->Unprotect();
#endif

        self->UninstallHandler(true);

#if USE_PROTECTED_ALLOCATIONS
        if (gBreakpadAllocator)
          gBreakpadAllocator->Protect();
#endif
        }
        
        
        
        ExceptionReplyMessage reply;
        if (!breakpad_exc_server(&receive.header, &reply.header))
          exit(1);

        
        mach_msg(&(reply.header), MACH_SEND_MSG,
                 reply.header.msgh_size, 0, MACH_PORT_NULL,
                 MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
      }
    }
  }

  return NULL;
}


void ExceptionHandler::SignalHandler(int sig, siginfo_t* info, void* uc) {
#if USE_PROTECTED_ALLOCATIONS
  if (gBreakpadAllocator)
    gBreakpadAllocator->Unprotect();
#endif
  gProtectedData.handler->WriteMinidumpWithException(
      EXC_SOFTWARE,
      MD_EXCEPTION_CODE_MAC_ABORT,
      0,
      static_cast<ucontext_t*>(uc),
      mach_thread_self(),
      true,
      true);
#if USE_PROTECTED_ALLOCATIONS
  if (gBreakpadAllocator)
    gBreakpadAllocator->Protect();
#endif
}

bool ExceptionHandler::InstallHandler() {
  
  if (gProtectedData.handler != NULL) {
    return false;
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGABRT);
  sa.sa_sigaction = ExceptionHandler::SignalHandler;
  sa.sa_flags = SA_SIGINFO;

  scoped_ptr<struct sigaction> old(new struct sigaction);
  if (sigaction(SIGABRT, &sa, old.get()) == -1) {
    return false;
  }
  old_handler_.swap(old);
  gProtectedData.handler = this;
#if USE_PROTECTED_ALLOCATIONS
  assert(((size_t)(gProtectedData.protected_buffer) & PAGE_MASK) == 0);
  mprotect(gProtectedData.protected_buffer, PAGE_SIZE, PROT_READ);
#endif

  try {
#if USE_PROTECTED_ALLOCATIONS
    previous_ = new (gBreakpadAllocator->Allocate(sizeof(ExceptionParameters)) )
      ExceptionParameters();
#else
    previous_ = new ExceptionParameters();
#endif
  }
  catch (std::bad_alloc) {
    return false;
  }

  
  previous_->count = EXC_TYPES_COUNT;
  mach_port_t current_task = mach_task_self();
  kern_return_t result = task_get_exception_ports(current_task,
                                                  s_exception_mask,
                                                  previous_->masks,
                                                  &previous_->count,
                                                  previous_->ports,
                                                  previous_->behaviors,
                                                  previous_->flavors);

  
  if (result == KERN_SUCCESS)
    result = task_set_exception_ports(current_task, s_exception_mask,
                                      handler_port_, EXCEPTION_DEFAULT,
                                      THREAD_STATE_NONE);

  installed_exception_handler_ = (result == KERN_SUCCESS);

  return installed_exception_handler_;
}

bool ExceptionHandler::UninstallHandler(bool in_exception) {
  kern_return_t result = KERN_SUCCESS;

  if (old_handler_.get()) {
    sigaction(SIGABRT, old_handler_.get(), NULL);
#if USE_PROTECTED_ALLOCATIONS
    mprotect(gProtectedData.protected_buffer, PAGE_SIZE,
        PROT_READ | PROT_WRITE);
#endif
    old_handler_.reset();
    gProtectedData.handler = NULL;
  }

  if (installed_exception_handler_) {
    mach_port_t current_task = mach_task_self();

    
    for (unsigned int i = 0; i < previous_->count; ++i) {
       result = task_set_exception_ports(current_task, previous_->masks[i],
                                        previous_->ports[i],
                                        previous_->behaviors[i],
                                        previous_->flavors[i]);
      if (result != KERN_SUCCESS)
        return false;
    }

    
    if (!in_exception) {
#if USE_PROTECTED_ALLOCATIONS
      previous_->~ExceptionParameters();
#else
      delete previous_;
#endif
    }

    previous_ = NULL;
    installed_exception_handler_ = false;
  }

  return result == KERN_SUCCESS;
}

bool ExceptionHandler::Setup(bool install_handler) {
  if (pthread_mutex_init(&minidump_write_mutex_, NULL))
    return false;

  
  mach_port_t current_task = mach_task_self();
  kern_return_t result = mach_port_allocate(current_task,
                                            MACH_PORT_RIGHT_RECEIVE,
                                            &handler_port_);
  
  if (result == KERN_SUCCESS)
    result = mach_port_insert_right(current_task, handler_port_, handler_port_,
                                    MACH_MSG_TYPE_MAKE_SEND);

  if (install_handler && result == KERN_SUCCESS)
    if (!InstallHandler())
      return false;

  if (result == KERN_SUCCESS) {
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int thread_create_result = pthread_create(&handler_thread_, &attr,
                                              &WaitForMessage, this);
    pthread_attr_destroy(&attr);
    result = thread_create_result ? KERN_FAILURE : KERN_SUCCESS;
  }

  return result == KERN_SUCCESS;
}

bool ExceptionHandler::Teardown() {
  kern_return_t result = KERN_SUCCESS;
  is_in_teardown_ = true;

  if (!UninstallHandler(false))
    return false;

  
  if (SendMessageToHandlerThread(kShutdownMessage)) {
    mach_port_t current_task = mach_task_self();
    result = mach_port_deallocate(current_task, handler_port_);
    if (result != KERN_SUCCESS)
      return false;
  } else {
    return false;
  }

  handler_thread_ = NULL;
  handler_port_ = MACH_PORT_NULL;
  pthread_mutex_destroy(&minidump_write_mutex_);

  return result == KERN_SUCCESS;
}

bool ExceptionHandler::SendMessageToHandlerThread(
    HandlerThreadMessage message_id) {
  ExceptionMessage msg;
  memset(&msg, 0, sizeof(msg));
  msg.header.msgh_id = message_id;
  if (message_id == kWriteDumpMessage ||
      message_id == kWriteDumpWithExceptionMessage) {
    
    msg.thread.name = mach_thread_self();
    msg.thread.disposition = MACH_MSG_TYPE_PORT_SEND;
    msg.thread.type = MACH_MSG_PORT_DESCRIPTOR;
  }
  msg.header.msgh_size = sizeof(msg) - sizeof(msg.padding);
  msg.header.msgh_remote_port = handler_port_;
  msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
                                          MACH_MSG_TYPE_MAKE_SEND_ONCE);
  kern_return_t result = mach_msg(&(msg.header),
                                  MACH_SEND_MSG | MACH_SEND_TIMEOUT,
                                  msg.header.msgh_size, 0, 0,
                                  MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

  return result == KERN_SUCCESS;
}

void ExceptionHandler::UpdateNextID() {
  next_minidump_path_ =
    (MinidumpGenerator::UniqueNameInDirectory(dump_path_, &next_minidump_id_));

  next_minidump_path_c_ = next_minidump_path_.c_str();
  next_minidump_id_c_ = next_minidump_id_.c_str();
}

bool ExceptionHandler::SuspendThreads() {
  thread_act_port_array_t   threads_for_task;
  mach_msg_type_number_t    thread_count;

  if (task_threads(mach_task_self(), &threads_for_task, &thread_count))
    return false;

  
  for (unsigned int i = 0; i < thread_count; ++i) {
    if (threads_for_task[i] != mach_thread_self()) {
      if (thread_suspend(threads_for_task[i]))
        return false;
    }
  }

  return true;
}

bool ExceptionHandler::ResumeThreads() {
  thread_act_port_array_t   threads_for_task;
  mach_msg_type_number_t    thread_count;

  if (task_threads(mach_task_self(), &threads_for_task, &thread_count))
    return false;

  
  for (unsigned int i = 0; i < thread_count; ++i) {
    if (threads_for_task[i] != mach_thread_self()) {
      if (thread_resume(threads_for_task[i]))
        return false;
    }
  }

  return true;
}

}  
