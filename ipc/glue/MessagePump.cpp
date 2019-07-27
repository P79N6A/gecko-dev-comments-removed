



#include "MessagePump.h"

#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsITimer.h"
#include "nsICancelableRunnable.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_nsautorelease_pool.h"
#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "nsComponentManagerUtils.h"
#include "nsDebug.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsTimerImpl.h"
#include "nsXULAppAPI.h"
#include "prthread.h"

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

using base::TimeTicks;
using namespace mozilla::ipc;

NS_DEFINE_NAMED_CID(NS_TIMER_CID);

static mozilla::DebugOnly<MessagePump::Delegate*> gFirstDelegate;

namespace mozilla {
namespace ipc {

class DoWorkRunnable final : public nsICancelableRunnable,
                             public nsITimerCallback
{
public:
  explicit DoWorkRunnable(MessagePump* aPump)
  : mPump(aPump)
  {
    MOZ_ASSERT(aPump);
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSICANCELABLERUNNABLE

private:
  ~DoWorkRunnable()
  { }

  MessagePump* mPump;
  
  
};

} 
} 

MessagePump::MessagePump()
: mThread(nullptr)
{
  mDoWorkEvent = new DoWorkRunnable(this);
}

MessagePump::~MessagePump()
{
}

void
MessagePump::Run(MessagePump::Delegate* aDelegate)
{
  MOZ_ASSERT(keep_running_);
  MOZ_ASSERT(NS_IsMainThread(),
             "Use mozilla::ipc::MessagePumpForNonMainThreads instead!");

  mThread = NS_GetCurrentThread();
  MOZ_ASSERT(mThread);

  mDelayedWorkTimer = do_CreateInstance(kNS_TIMER_CID);
  MOZ_ASSERT(mDelayedWorkTimer);

  base::ScopedNSAutoreleasePool autoReleasePool;

  for (;;) {
    autoReleasePool.Recycle();

    bool did_work = NS_ProcessNextEvent(mThread, false) ? true : false;
    if (!keep_running_)
      break;

    
    
    
    

    did_work |= aDelegate->DoDelayedWork(&delayed_work_time_);

if (did_work && delayed_work_time_.is_null()
#ifdef MOZ_NUWA_PROCESS
    && (!IsNuwaReady() || !IsNuwaProcess())
#endif
   )
      mDelayedWorkTimer->Cancel();

    if (!keep_running_)
      break;

    if (did_work)
      continue;

    did_work = aDelegate->DoIdleWork();
    if (!keep_running_)
      break;

    if (did_work)
      continue;

    
    NS_ProcessNextEvent(mThread, true);
  }

#ifdef MOZ_NUWA_PROCESS
  if (!IsNuwaReady() || !IsNuwaProcess())
#endif
    mDelayedWorkTimer->Cancel();

  keep_running_ = true;
}

void
MessagePump::ScheduleWork()
{
  
  if (mThread) {
    mThread->Dispatch(mDoWorkEvent, NS_DISPATCH_NORMAL);
  }
  else {
    
    
    NS_DispatchToMainThread(mDoWorkEvent);
  }
  event_.Signal();
}

void
MessagePump::ScheduleWorkForNestedLoop()
{
  
  
  
  
}

void
MessagePump::ScheduleDelayedWork(const base::TimeTicks& aDelayedTime)
{
#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaReady() && IsNuwaProcess())
    return;
#endif

  if (!mDelayedWorkTimer) {
    mDelayedWorkTimer = do_CreateInstance(kNS_TIMER_CID);
    if (!mDelayedWorkTimer) {
        
        NS_WARNING("Delayed task might not run!");
        delayed_work_time_ = aDelayedTime;
        return;
    }
  }

  if (!delayed_work_time_.is_null()) {
    mDelayedWorkTimer->Cancel();
  }

  delayed_work_time_ = aDelayedTime;

  
  base::TimeDelta delay;
  if (aDelayedTime > base::TimeTicks::Now())
    delay = aDelayedTime - base::TimeTicks::Now();

  uint32_t delayMS = uint32_t(delay.InMilliseconds());
  mDelayedWorkTimer->InitWithCallback(mDoWorkEvent, delayMS,
                                      nsITimer::TYPE_ONE_SHOT);
}

void
MessagePump::DoDelayedWork(base::MessagePump::Delegate* aDelegate)
{
  aDelegate->DoDelayedWork(&delayed_work_time_);
  if (!delayed_work_time_.is_null()) {
    ScheduleDelayedWork(delayed_work_time_);
  }
}

NS_IMPL_ISUPPORTS(DoWorkRunnable, nsIRunnable, nsITimerCallback,
                                  nsICancelableRunnable)

NS_IMETHODIMP
DoWorkRunnable::Run()
{
  MessageLoop* loop = MessageLoop::current();
  MOZ_ASSERT(loop);

  bool nestableTasksAllowed = loop->NestableTasksAllowed();

  
  
  
  loop->SetNestableTasksAllowed(true);
  loop->DoWork();
  loop->SetNestableTasksAllowed(nestableTasksAllowed);

  return NS_OK;
}

NS_IMETHODIMP
DoWorkRunnable::Notify(nsITimer* aTimer)
{
  MessageLoop* loop = MessageLoop::current();
  MOZ_ASSERT(loop);

  mPump->DoDelayedWork(loop);

  return NS_OK;
}

NS_IMETHODIMP
DoWorkRunnable::Cancel()
{
  
  
  
  
  
  
  
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(Run()));
  return NS_OK;
}

void
MessagePumpForChildProcess::Run(base::MessagePump::Delegate* aDelegate)
{
  if (mFirstRun) {
    MOZ_ASSERT(aDelegate && !gFirstDelegate);
    gFirstDelegate = aDelegate;

    mFirstRun = false;
    if (NS_FAILED(XRE_RunAppShell())) {
        NS_WARNING("Failed to run app shell?!");
    }

    MOZ_ASSERT(aDelegate && aDelegate == gFirstDelegate);
    gFirstDelegate = nullptr;

    return;
  }

  MOZ_ASSERT(aDelegate && aDelegate == gFirstDelegate);

  
  
  
  
  
  
  
  
  
  MessageLoop* loop = MessageLoop::current();
  bool nestableTasksAllowed = loop->NestableTasksAllowed();
  loop->SetNestableTasksAllowed(true);

  while (aDelegate->DoWork());

  loop->SetNestableTasksAllowed(nestableTasksAllowed);

  
  mozilla::ipc::MessagePump::Run(aDelegate);
}

void
MessagePumpForNonMainThreads::Run(base::MessagePump::Delegate* aDelegate)
{
  MOZ_ASSERT(keep_running_);
  MOZ_ASSERT(!NS_IsMainThread(), "Use mozilla::ipc::MessagePump instead!");

  mThread = NS_GetCurrentThread();
  MOZ_ASSERT(mThread);

  mDelayedWorkTimer = do_CreateInstance(kNS_TIMER_CID);
  MOZ_ASSERT(mDelayedWorkTimer);

  if (NS_FAILED(mDelayedWorkTimer->SetTarget(mThread))) {
    MOZ_CRASH("Failed to set timer target!");
  }

  
  
  
  
  
  
  
  
  
  
  
  while (aDelegate->DoWork()) {
  }

  base::ScopedNSAutoreleasePool autoReleasePool;
  for (;;) {
    autoReleasePool.Recycle();

    bool didWork = NS_ProcessNextEvent(mThread, false) ? true : false;
    if (!keep_running_) {
      break;
    }

    didWork |= aDelegate->DoDelayedWork(&delayed_work_time_);

    if (didWork && delayed_work_time_.is_null()) {
      mDelayedWorkTimer->Cancel();
    }

    if (!keep_running_) {
      break;
    }

    if (didWork) {
      continue;
    }

    didWork = aDelegate->DoIdleWork();
    if (!keep_running_) {
      break;
    }

    if (didWork) {
      continue;
    }

    
    NS_ProcessNextEvent(mThread, true);
  }

  mDelayedWorkTimer->Cancel();

  keep_running_ = true;
}

#if defined(XP_WIN)

NS_IMPL_QUERY_INTERFACE(MessagePumpForNonMainUIThreads, nsIThreadObserver)

#define CHECK_QUIT_STATE { if (state_->should_quit) { break; } }

void MessagePumpForNonMainUIThreads::DoRunLoop()
{
  
  
  mThread = NS_GetCurrentThread();
  MOZ_ASSERT(mThread);

  
  
  nsCOMPtr<nsIThreadInternal> ti(do_QueryInterface(mThread));
  MOZ_ASSERT(ti);
  ti->SetObserver(this);

  base::ScopedNSAutoreleasePool autoReleasePool;
  for (;;) {
    autoReleasePool.Recycle();

    bool didWork = NS_ProcessNextEvent(mThread, false);

    didWork |= ProcessNextWindowsMessage();
    CHECK_QUIT_STATE

    didWork |= state_->delegate->DoWork();
    CHECK_QUIT_STATE

    didWork |= state_->delegate->DoDelayedWork(&delayed_work_time_);
    if (didWork && delayed_work_time_.is_null()) {
      KillTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this));
    }
    CHECK_QUIT_STATE

    if (didWork) {
      continue;
    }

    didWork = state_->delegate->DoIdleWork();
    CHECK_QUIT_STATE

    SetInWait();
    bool hasWork = NS_HasPendingEvents(mThread);
    if (didWork || hasWork) {
      ClearInWait();
      continue;
    }
    WaitForWork(); 
    ClearInWait();
  }

  ClearInWait();

  ti->SetObserver(nullptr);
}

NS_IMETHODIMP
MessagePumpForNonMainUIThreads::OnDispatchedEvent(nsIThreadInternal *thread)
{
  
  
  
  if (GetInWait()) {
    ScheduleWork();
  }
  return NS_OK;
}

NS_IMETHODIMP
MessagePumpForNonMainUIThreads::OnProcessNextEvent(nsIThreadInternal *thread,
                                                   bool mayWait,
                                                   uint32_t recursionDepth)
{
  return NS_OK;
}

NS_IMETHODIMP
MessagePumpForNonMainUIThreads::AfterProcessNextEvent(nsIThreadInternal *thread,
                                                      uint32_t recursionDepth,
                                                      bool eventWasProcessed)
{
  return NS_OK;
}

#endif 
