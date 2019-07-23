



#include "base/platform_thread.h"

#include "base/logging.h"
#include "base/win_util.h"

namespace {



const DWORD kVCThreadNameException = 0x406D1388;

typedef struct tagTHREADNAME_INFO {
  DWORD dwType;  
  LPCSTR szName;  
  DWORD dwThreadID;  
  DWORD dwFlags;  
} THREADNAME_INFO;

DWORD __stdcall ThreadFunc(void* closure) {
  PlatformThread::Delegate* delegate =
      static_cast<PlatformThread::Delegate*>(closure);
  delegate->ThreadMain();
  return NULL;
}

}  


PlatformThreadId PlatformThread::CurrentId() {
  return GetCurrentThreadId();
}


void PlatformThread::YieldCurrentThread() {
  ::Sleep(0);
}


void PlatformThread::Sleep(int duration_ms) {
  ::Sleep(duration_ms);
}


void PlatformThread::SetName(const char* name) {
  
  
  if (!::IsDebuggerPresent())
    return;

  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = name;
  info.dwThreadID = CurrentId();
  info.dwFlags = 0;

  __try {
    RaiseException(kVCThreadNameException, 0, sizeof(info)/sizeof(DWORD),
                   reinterpret_cast<DWORD_PTR*>(&info));
  } __except(EXCEPTION_CONTINUE_EXECUTION) {
  }
}


bool PlatformThread::Create(size_t stack_size, Delegate* delegate,
                            PlatformThreadHandle* thread_handle) {
  unsigned int flags = 0;
  if (stack_size > 0 && win_util::GetWinVersion() >= win_util::WINVERSION_XP) {
    flags = STACK_SIZE_PARAM_IS_A_RESERVATION;
  } else {
    stack_size = 0;
  }

  
  
  
  
  
  *thread_handle = CreateThread(
      NULL, stack_size, ThreadFunc, delegate, flags, NULL);
  return *thread_handle != NULL;
}


bool PlatformThread::CreateNonJoinable(size_t stack_size, Delegate* delegate) {
  PlatformThreadHandle thread_handle;
  bool result = Create(stack_size, delegate, &thread_handle);
  CloseHandle(thread_handle);
  return result;
}


void PlatformThread::Join(PlatformThreadHandle thread_handle) {
  DCHECK(thread_handle);

  
  
  DWORD result = WaitForSingleObject(thread_handle, INFINITE);
  DCHECK_EQ(WAIT_OBJECT_0, result);

  CloseHandle(thread_handle);
}
