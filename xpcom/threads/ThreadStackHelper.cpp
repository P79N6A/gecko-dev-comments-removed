




#include "ThreadStackHelper.h"
#include "MainThreadUtils.h"

#include "mozilla/Assertions.h"
#include "mozilla/Move.h"

#ifdef XP_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#endif

#ifdef ANDROID
#ifndef SYS_gettid
#define SYS_gettid __NR_gettid
#endif
#ifndef SYS_tgkill
#define SYS_tgkill __NR_tgkill
#endif
#endif

namespace mozilla {

void
ThreadStackHelper::Startup()
{
#if defined(XP_LINUX)
  MOZ_ASSERT(NS_IsMainThread());
  if (!sInitialized) {
    MOZ_ALWAYS_TRUE(!::sem_init(&sSem, 0, 0));
  }
  sInitialized++;
#endif
}

void
ThreadStackHelper::Shutdown()
{
#if defined(XP_LINUX)
  MOZ_ASSERT(NS_IsMainThread());
  if (sInitialized == 1) {
    MOZ_ALWAYS_TRUE(!::sem_destroy(&sSem));
  }
  sInitialized--;
#endif
}

ThreadStackHelper::ThreadStackHelper()
  :
#ifdef MOZ_ENABLE_PROFILER_SPS
    mPseudoStack(mozilla_get_pseudo_stack()),
#endif
    mStackBuffer()
  , mMaxStackSize(mStackBuffer.capacity())
{
#if defined(XP_LINUX)
  mThreadID = ::syscall(SYS_gettid);
#elif defined(XP_WIN)
  mInitialized = !!::DuplicateHandle(
    ::GetCurrentProcess(), ::GetCurrentThread(),
    ::GetCurrentProcess(), &mThreadID,
    THREAD_SUSPEND_RESUME, FALSE, 0);
  MOZ_ASSERT(mInitialized);
#elif defined(XP_MACOSX)
  mThreadID = mach_thread_self();
#endif
}

ThreadStackHelper::~ThreadStackHelper()
{
#if defined(XP_WIN)
  if (mInitialized) {
    MOZ_ALWAYS_TRUE(!!::CloseHandle(mThreadID));
  }
#endif
}

#if defined(XP_LINUX) && defined(__arm__)








template <void (*H)(int, siginfo_t*, void*)>
__attribute__((naked)) void
SignalTrampoline(int aSignal, siginfo_t* aInfo, void* aContext)
{
  asm volatile (
    "nop; nop; nop; nop"
    : : : "memory");

  
  

  asm volatile (
    "bx %0"
    :
    : "r"(H), "l"(aSignal), "l"(aInfo), "l"(aContext)
    : "memory");
}
#endif 

void
ThreadStackHelper::GetStack(Stack& aStack)
{
  
  if (!PrepareStackBuffer(aStack)) {
    
    return;
  }

#if defined(XP_LINUX)
  if (profiler_is_active()) {
    
    return;
  }
  if (!sInitialized) {
    MOZ_ASSERT(false);
    return;
  }
  sCurrent = this;
  struct sigaction sigact = {};
#ifdef __arm__
  sigact.sa_sigaction = SignalTrampoline<SigAction>;
#else
  sigact.sa_sigaction = SigAction;
#endif
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = SA_SIGINFO | SA_RESTART;
  if (::sigaction(SIGPROF, &sigact, &sOldSigAction)) {
    MOZ_ASSERT(false);
    return;
  }
  MOZ_ALWAYS_TRUE(!::syscall(SYS_tgkill, getpid(), mThreadID, SIGPROF));
  MOZ_ALWAYS_TRUE(!::sem_wait(&sSem));

#elif defined(XP_WIN)
  if (!mInitialized) {
    MOZ_ASSERT(false);
    return;
  }
  if (::SuspendThread(mThreadID) == DWORD(-1)) {
    MOZ_ASSERT(false);
    return;
  }
  FillStackBuffer();
  MOZ_ALWAYS_TRUE(::ResumeThread(mThreadID) != DWORD(-1));

#elif defined(XP_MACOSX)
  if (::thread_suspend(mThreadID) != KERN_SUCCESS) {
    MOZ_ASSERT(false);
    return;
  }
  FillStackBuffer();
  MOZ_ALWAYS_TRUE(::thread_resume(mThreadID) == KERN_SUCCESS);

#endif
  aStack = Move(mStackBuffer);
}

#ifdef XP_LINUX

int ThreadStackHelper::sInitialized;
sem_t ThreadStackHelper::sSem;
struct sigaction ThreadStackHelper::sOldSigAction;
ThreadStackHelper* ThreadStackHelper::sCurrent;

void
ThreadStackHelper::SigAction(int aSignal, siginfo_t* aInfo, void* aContext)
{
  ::sigaction(SIGPROF, &sOldSigAction, nullptr);
  sCurrent->FillStackBuffer();
  sCurrent = nullptr;
  ::sem_post(&sSem);
}

#endif 

bool
ThreadStackHelper::PrepareStackBuffer(Stack& aStack)
{
  
  aStack.clear();
#ifdef MOZ_ENABLE_PROFILER_SPS
  




#ifdef MOZ_WIDGET_GONK
  if (!mPseudoStack) {
    return false;
  }
#endif
  MOZ_ASSERT(mPseudoStack);
  mStackBuffer.clear();
  MOZ_ALWAYS_TRUE(mStackBuffer.reserve(mMaxStackSize));
  return true;
#else
  return false;
#endif
}

void
ThreadStackHelper::FillStackBuffer()
{
#ifdef MOZ_ENABLE_PROFILER_SPS
  size_t reservedSize = mMaxStackSize;

  
  const volatile StackEntry* entry = mPseudoStack->mStack;
  const volatile StackEntry* end = entry + mPseudoStack->stackSize();
  
  const char* prevLabel = nullptr;
  for (; reservedSize-- && entry != end; entry++) {
    

    if (entry->isCopyLabel()) {
      continue;
    }
    const char* const label = entry->label();
    if (label == prevLabel) {
      continue;
    }
    mStackBuffer.infallibleAppend(label);
    prevLabel = label;
  }
  
  mMaxStackSize += (end - entry);
#endif
}

} 
