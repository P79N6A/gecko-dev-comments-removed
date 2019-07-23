




























#include <map>
#include <pthread.h>

#include "client/mac/handler/exception_handler.h"
#include "client/mac/handler/minidump_generator.h"
#include "common/mac/macho_utilities.h"

#ifndef USE_PROTECTED_ALLOCATIONS
#define USE_PROTECTED_ALLOCATIONS 0
#endif





#if USE_PROTECTED_ALLOCATIONS
  #include "protected_memory_allocator.h"
  extern ProtectedMemoryAllocator *gBreakpadAllocator;
#endif


namespace google_breakpad {

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

extern "C"
{
  
  boolean_t exc_server(mach_msg_header_t *request,
                       mach_msg_header_t *reply);

  kern_return_t catch_exception_raise(mach_port_t target_port,
                                      mach_port_t failed_thread,
                                      mach_port_t task,
                                      exception_type_t exception,
                                      exception_data_t code,
                                      mach_msg_type_number_t code_count);

  kern_return_t ForwardException(mach_port_t task,
                                 mach_port_t failed_thread,
                                 exception_type_t exception,
                                 exception_data_t code,
                                 mach_msg_type_number_t code_count);

  kern_return_t exception_raise(mach_port_t target_port,
                                mach_port_t failed_thread,
                                mach_port_t task,
                                exception_type_t exception,
                                exception_data_t exception_code,
                                mach_msg_type_number_t exception_code_count);

  kern_return_t
    exception_raise_state(mach_port_t target_port,
                          mach_port_t failed_thread,
                          mach_port_t task,
                          exception_type_t exception,
                          exception_data_t exception_code,
                          mach_msg_type_number_t code_count,
                          thread_state_flavor_t *target_flavor,
                          thread_state_t thread_state,
                          mach_msg_type_number_t thread_state_count,
                          thread_state_t thread_state,
                          mach_msg_type_number_t *thread_state_count);

  kern_return_t
    exception_raise_state_identity(mach_port_t target_port,
                                   mach_port_t failed_thread,
                                   mach_port_t task,
                                   exception_type_t exception,
                                   exception_data_t exception_code,
                                   mach_msg_type_number_t exception_code_count,
                                   thread_state_flavor_t *target_flavor,
                                   thread_state_t thread_state,
                                   mach_msg_type_number_t thread_state_count,
                                   thread_state_t thread_state,
                                   mach_msg_type_number_t *thread_state_count);

  kern_return_t breakpad_exception_raise_state(mach_port_t exception_port,
                                               exception_type_t exception,
                                               const exception_data_t code,
                                               mach_msg_type_number_t codeCnt,
                                               int *flavor,
                                               const thread_state_t old_state,
                                               mach_msg_type_number_t old_stateCnt,
                                               thread_state_t new_state,
                                               mach_msg_type_number_t *new_stateCnt
                                               );

  kern_return_t breakpad_exception_raise_state_identity(mach_port_t exception_port,
                                                        mach_port_t thread,
                                                        mach_port_t task,
                                                        exception_type_t exception,
                                                        exception_data_t code,
                                                        mach_msg_type_number_t codeCnt,
                                                        int *flavor,
                                                        thread_state_t old_state,
                                                        mach_msg_type_number_t old_stateCnt,
                                                        thread_state_t new_state,
                                                        mach_msg_type_number_t *new_stateCnt
                                                        );

  kern_return_t breakpad_exception_raise(mach_port_t port, mach_port_t failed_thread,
                                         mach_port_t task,
                                         exception_type_t exception,
                                         exception_data_t code,
                                         mach_msg_type_number_t code_count);
}



kern_return_t breakpad_exception_raise_state(mach_port_t exception_port,
					     exception_type_t exception,
					     const exception_data_t code,
					     mach_msg_type_number_t codeCnt,
					     int *flavor,
					     const thread_state_t old_state,
					     mach_msg_type_number_t old_stateCnt,
					     thread_state_t new_state,
					     mach_msg_type_number_t *new_stateCnt
                                             )
{
  return KERN_SUCCESS;
}

kern_return_t breakpad_exception_raise_state_identity(mach_port_t exception_port,
						      mach_port_t thread,
						      mach_port_t task,
						      exception_type_t exception,
						      exception_data_t code,
						      mach_msg_type_number_t codeCnt,
						      int *flavor,
						      thread_state_t old_state,
						      mach_msg_type_number_t old_stateCnt,
						      thread_state_t new_state,
						      mach_msg_type_number_t *new_stateCnt
                                                      )
{
  return KERN_SUCCESS;
}

kern_return_t breakpad_exception_raise(mach_port_t port, mach_port_t failed_thread,
                                       mach_port_t task,
                                       exception_type_t exception,
                                       exception_data_t code,
                                       mach_msg_type_number_t code_count) {

  if (task != mach_task_self()) {
    return KERN_FAILURE;
  }
  return ForwardException(task, failed_thread, exception, code, code_count);
}


ExceptionHandler::ExceptionHandler(const string &dump_path,
                                   FilterCallback filter,
                                   MinidumpCallback callback,
                                   void *callback_context,
                                   bool install_handler)
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
  Setup(install_handler);
}



ExceptionHandler::ExceptionHandler(DirectCallback callback,
                                   void *callback_context,
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

bool ExceptionHandler::WriteMinidump() {
  
  if (use_minidump_write_mutex_)
    return false;

  use_minidump_write_mutex_ = true;
  last_minidump_write_result_ = false;

  
  if (pthread_mutex_lock(&minidump_write_mutex_) == 0) {
    
    
    SendEmptyMachMessage();

    
    
    pthread_mutex_lock(&minidump_write_mutex_);
  }

  use_minidump_write_mutex_ = false;
  UpdateNextID();
  return last_minidump_write_result_;
}


bool ExceptionHandler::WriteMinidump(const string &dump_path,
                                     MinidumpCallback callback,
                                     void *callback_context) {
  ExceptionHandler handler(dump_path, NULL, callback, callback_context, false);
  return handler.WriteMinidump();
}

bool ExceptionHandler::WriteMinidumpWithException(int exception_type,
                                                  int exception_code,
                                                  int exception_subcode,
                                                  mach_port_t thread_name) {
  bool result = false;

  if (directCallback_) {
    if (directCallback_(callback_context_,
                        exception_type,
                        exception_code,
                        exception_subcode,
                        thread_name) ) {
      if (exception_type && exception_code)
        _exit(exception_type);
    }
  } else {
    string minidump_id;

    
    
    if (!dump_path_.empty()) {
      MinidumpGenerator md;
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
        if (exception_type && exception_code)
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
  kern_return_t result = task_get_exception_ports(current_task,
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
  thread_state_flavor_t target_flavor = current.flavors[found];

  mach_msg_type_number_t thread_state_count = THREAD_STATE_MAX;
  breakpad_thread_state_data_t thread_state;
  switch (target_behavior) {
    case EXCEPTION_DEFAULT:
      result = exception_raise(target_port, failed_thread, task, exception,
                               code, code_count);
      break;

    case EXCEPTION_STATE:
      result = thread_get_state(failed_thread, target_flavor, thread_state,
                                &thread_state_count);
      if (result == KERN_SUCCESS)
        result = exception_raise_state(target_port, failed_thread, task,
                                       exception, code,
                                       code_count, &target_flavor,
                                       thread_state, thread_state_count,
                                       thread_state, &thread_state_count);
      if (result == KERN_SUCCESS)
        result = thread_set_state(failed_thread, target_flavor, thread_state,
                                  thread_state_count);
      break;

    case EXCEPTION_STATE_IDENTITY:
      result = thread_get_state(failed_thread, target_flavor, thread_state,
                                &thread_state_count);
      if (result == KERN_SUCCESS)
        result = exception_raise_state_identity(target_port, failed_thread,
                                                task, exception, code,
                                                code_count, &target_flavor,
                                                thread_state,
                                                thread_state_count,
                                                thread_state,
                                                &thread_state_count);
      if (result == KERN_SUCCESS)
        result = thread_set_state(failed_thread, target_flavor, thread_state,
                                  thread_state_count);
      break;

    default:
      fprintf(stderr, "** Unknown exception behavior\n");
      result = KERN_FAILURE;
      break;
  }

  return result;
}


kern_return_t catch_exception_raise(mach_port_t port, mach_port_t failed_thread,
                                    mach_port_t task,
                                    exception_type_t exception,
                                    exception_data_t code,
                                    mach_msg_type_number_t code_count) {
  return ForwardException(task, failed_thread, exception, code, code_count);
}


void *ExceptionHandler::WaitForMessage(void *exception_handler_class) {
  ExceptionHandler *self =
    reinterpret_cast<ExceptionHandler *>(exception_handler_class);
  ExceptionMessage receive;

  
  while (1) {
    receive.header.msgh_local_port = self->handler_port_;
    receive.header.msgh_size = sizeof(receive);
    kern_return_t result = mach_msg(&(receive.header),
                                    MACH_RCV_MSG | MACH_RCV_LARGE, 0,
                                    sizeof(receive), self->handler_port_,
                                    MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);


    if (result == KERN_SUCCESS) {
      
      
      

      
      
      
      
      
      
      
      if (!receive.exception) {
        if (self->is_in_teardown_)
          return NULL;

        self->SuspendThreads();

#if USE_PROTECTED_ALLOCATIONS
        if(gBreakpadAllocator)
          gBreakpadAllocator->Unprotect();
#endif

        
        self->last_minidump_write_result_ =
          self->WriteMinidumpWithException(0, 0, 0, 0);

        self->UninstallHandler(false);

#if USE_PROTECTED_ALLOCATIONS
        if(gBreakpadAllocator)
          gBreakpadAllocator->Protect();
#endif

        self->ResumeThreads();

        if (self->use_minidump_write_mutex_)
          pthread_mutex_unlock(&self->minidump_write_mutex_);
      } else {
        
        
        
        
        
        
        
        
        if (receive.task.name == mach_task_self()) {
          self->SuspendThreads();

#if USE_PROTECTED_ALLOCATIONS
        if(gBreakpadAllocator)
          gBreakpadAllocator->Unprotect();
#endif

        int subcode = 0;
        if (receive.exception == EXC_BAD_ACCESS && receive.code_count > 1)
          subcode = receive.code[1];

        
        self->WriteMinidumpWithException(receive.exception, receive.code[0],
                                         subcode, receive.thread.name);

        self->UninstallHandler(true);

#if USE_PROTECTED_ALLOCATIONS
        if(gBreakpadAllocator)
          gBreakpadAllocator->Protect();
#endif
        }
        
        
        
        ExceptionReplyMessage reply;
        if (!exc_server(&receive.header, &reply.header))
          exit(1);

        
        result = mach_msg(&(reply.header), MACH_SEND_MSG,
                          reply.header.msgh_size, 0, MACH_PORT_NULL,
                          MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
      }
    }
  }

  return NULL;
}

bool ExceptionHandler::InstallHandler() {
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

  return result == KERN_SUCCESS ? true : false;
}

bool ExceptionHandler::Teardown() {
  kern_return_t result = KERN_SUCCESS;
  is_in_teardown_ = true;

  if (!UninstallHandler(false))
    return false;

  
  if (SendEmptyMachMessage()) {
    mach_port_t current_task = mach_task_self();
    result = mach_port_deallocate(current_task, handler_port_);
    if (result != KERN_SUCCESS)
      return false;
  } else {
    return false;
  }

  handler_thread_ = NULL;
  handler_port_ = NULL;
  pthread_mutex_destroy(&minidump_write_mutex_);

  return result == KERN_SUCCESS;
}

bool ExceptionHandler::SendEmptyMachMessage() {
  ExceptionMessage empty;
  memset(&empty, 0, sizeof(empty));
  empty.header.msgh_size = sizeof(empty) - sizeof(empty.padding);
  empty.header.msgh_remote_port = handler_port_;
  empty.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
                                          MACH_MSG_TYPE_MAKE_SEND_ONCE);
  kern_return_t result = mach_msg(&(empty.header),
                                  MACH_SEND_MSG | MACH_SEND_TIMEOUT,
                                  empty.header.msgh_size, 0, 0,
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
