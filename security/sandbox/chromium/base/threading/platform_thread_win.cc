



#include "base/threading/platform_thread.h"

#include "base/debug/alias.h"
#include "base/debug/profiler.h"
#include "base/logging.h"
#include "base/threading/thread_id_name_manager.h"
#include "base/threading/thread_restrictions.h"
#include "base/tracked_objects.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"

namespace base {

namespace {



const DWORD kVCThreadNameException = 0x406D1388;

typedef struct tagTHREADNAME_INFO {
  DWORD dwType;  
  LPCSTR szName;  
  DWORD dwThreadID;  
  DWORD dwFlags;  
} THREADNAME_INFO;


void SetNameInternal(PlatformThreadId thread_id, const char* name) {
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = name;
  info.dwThreadID = thread_id;
  info.dwFlags = 0;

  __try {
    RaiseException(kVCThreadNameException, 0, sizeof(info)/sizeof(DWORD),
                   reinterpret_cast<DWORD_PTR*>(&info));
  } __except(EXCEPTION_CONTINUE_EXECUTION) {
  }
}

struct ThreadParams {
  PlatformThread::Delegate* delegate;
  bool joinable;
};

DWORD __stdcall ThreadFunc(void* params) {
  ThreadParams* thread_params = static_cast<ThreadParams*>(params);
  PlatformThread::Delegate* delegate = thread_params->delegate;
  if (!thread_params->joinable)
    base::ThreadRestrictions::SetSingletonAllowed(false);

  
  
  PlatformThreadHandle::Handle platform_handle;
  BOOL did_dup = DuplicateHandle(GetCurrentProcess(),
                                GetCurrentThread(),
                                GetCurrentProcess(),
                                &platform_handle,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS);

  win::ScopedHandle scoped_platform_handle;

  if (did_dup) {
    scoped_platform_handle.Set(platform_handle);
    ThreadIdNameManager::GetInstance()->RegisterThread(
        scoped_platform_handle.Get(),
        PlatformThread::CurrentId());
  }

  delete thread_params;
  delegate->ThreadMain();

  if (did_dup) {
    ThreadIdNameManager::GetInstance()->RemoveName(
        scoped_platform_handle.Get(),
        PlatformThread::CurrentId());
  }

  return NULL;
}




bool CreateThreadInternal(size_t stack_size,
                          PlatformThread::Delegate* delegate,
                          PlatformThreadHandle* out_thread_handle) {
  unsigned int flags = 0;
  if (stack_size > 0 && base::win::GetVersion() >= base::win::VERSION_XP) {
    flags = STACK_SIZE_PARAM_IS_A_RESERVATION;
  } else {
    stack_size = 0;
  }

  ThreadParams* params = new ThreadParams;
  params->delegate = delegate;
  params->joinable = out_thread_handle != NULL;

  
  
  
  
  
  void* thread_handle = CreateThread(
      NULL, stack_size, ThreadFunc, params, flags, NULL);
  if (!thread_handle) {
    delete params;
    return false;
  }

  if (out_thread_handle)
    *out_thread_handle = PlatformThreadHandle(thread_handle);
  else
    CloseHandle(thread_handle);
  return true;
}

}  


PlatformThreadId PlatformThread::CurrentId() {
  return GetCurrentThreadId();
}


PlatformThreadRef PlatformThread::CurrentRef() {
  return PlatformThreadRef(GetCurrentThreadId());
}


PlatformThreadHandle PlatformThread::CurrentHandle() {
  NOTIMPLEMENTED(); 
  return PlatformThreadHandle();
}


void PlatformThread::YieldCurrentThread() {
  ::Sleep(0);
}


void PlatformThread::Sleep(TimeDelta duration) {
  
  
  TimeTicks end = TimeTicks::Now() + duration;
  for (TimeTicks now = TimeTicks::Now(); now < end; now = TimeTicks::Now())
    ::Sleep(static_cast<DWORD>((end - now).InMillisecondsRoundedUp()));
}


void PlatformThread::SetName(const char* name) {
  ThreadIdNameManager::GetInstance()->SetName(CurrentId(), name);

  
  
  
  
  
  
  if (0 != strcmp(name, "BrokerEvent"))
    tracked_objects::ThreadData::InitializeThreadContext(name);

  
  
  
  
  if (!::IsDebuggerPresent() && !base::debug::IsBinaryInstrumented())
    return;

  SetNameInternal(CurrentId(), name);
}


const char* PlatformThread::GetName() {
  return ThreadIdNameManager::GetInstance()->GetName(CurrentId());
}


bool PlatformThread::Create(size_t stack_size, Delegate* delegate,
                            PlatformThreadHandle* thread_handle) {
  DCHECK(thread_handle);
  return CreateThreadInternal(stack_size, delegate, thread_handle);
}


bool PlatformThread::CreateWithPriority(size_t stack_size, Delegate* delegate,
                                        PlatformThreadHandle* thread_handle,
                                        ThreadPriority priority) {
  bool result = Create(stack_size, delegate, thread_handle);
  if (result)
    SetThreadPriority(*thread_handle, priority);
  return result;
}


bool PlatformThread::CreateNonJoinable(size_t stack_size, Delegate* delegate) {
  return CreateThreadInternal(stack_size, delegate, NULL);
}


void PlatformThread::Join(PlatformThreadHandle thread_handle) {
  DCHECK(thread_handle.handle_);
  
  
  
  
  
#if 0
  base::ThreadRestrictions::AssertIOAllowed();
#endif

  
  
  DWORD result = WaitForSingleObject(thread_handle.handle_, INFINITE);
  if (result != WAIT_OBJECT_0) {
    
    DWORD error = GetLastError();
    debug::Alias(&error);
    debug::Alias(&result);
    debug::Alias(&thread_handle.handle_);
    CHECK(false);
  }

  CloseHandle(thread_handle.handle_);
}


void PlatformThread::SetThreadPriority(PlatformThreadHandle handle,
                                       ThreadPriority priority) {
  switch (priority) {
    case kThreadPriority_Normal:
      ::SetThreadPriority(handle.handle_, THREAD_PRIORITY_NORMAL);
      break;
    case kThreadPriority_RealtimeAudio:
      ::SetThreadPriority(handle.handle_, THREAD_PRIORITY_TIME_CRITICAL);
      break;
    default:
      NOTREACHED() << "Unknown priority.";
      break;
  }
}

}  
