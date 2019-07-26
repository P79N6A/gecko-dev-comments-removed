



#include "MessagePump.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "pratom.h"
#include "prthread.h"

#include "base/logging.h"
#include "base/scoped_nsautorelease_pool.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#endif

using mozilla::ipc::DoWorkRunnable;
using mozilla::ipc::MessagePump;
using mozilla::ipc::MessagePumpForChildProcess;
using base::TimeTicks;

NS_IMPL_THREADSAFE_ISUPPORTS2(DoWorkRunnable, nsIRunnable, nsITimerCallback)

NS_IMETHODIMP
DoWorkRunnable::Run()
{
  MessageLoop* loop = MessageLoop::current();
  NS_ASSERTION(loop, "Shouldn't be null!");
  if (loop) {
    bool nestableTasksAllowed = loop->NestableTasksAllowed();

    
    
    
    
    loop->SetNestableTasksAllowed(true);
    loop->DoWork();
    loop->SetNestableTasksAllowed(nestableTasksAllowed);
  }
  return NS_OK;
}

NS_IMETHODIMP
DoWorkRunnable::Notify(nsITimer* aTimer)
{
  MessageLoop* loop = MessageLoop::current();
  NS_ASSERTION(loop, "Shouldn't be null!");
  if (loop) {
    mPump->DoDelayedWork(loop);
  }
  return NS_OK;
}

MessagePump::MessagePump()
: mThread(nullptr)
{
  mDoWorkEvent = new DoWorkRunnable(this);
}

void
MessagePump::Run(MessagePump::Delegate* aDelegate)
{
  NS_ASSERTION(keep_running_, "Quit must have been called outside of Run!");
  NS_ASSERTION(NS_IsMainThread(), "Called Run on the wrong thread!");

  mThread = NS_GetCurrentThread();
  NS_ASSERTION(mThread, "This should never be null!");

  mDelayedWorkTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ASSERTION(mDelayedWorkTimer, "Failed to create timer!");

  base::ScopedNSAutoreleasePool autoReleasePool;

  for (;;) {
    autoReleasePool.Recycle();

    bool did_work = NS_ProcessNextEvent(mThread, false) ? true : false;
    if (!keep_running_)
      break;

    
    
    
    

#ifdef MOZ_WIDGET_ANDROID
    
    
    
    did_work |= AndroidBridge::Bridge()->PumpMessageLoop();
#endif

    did_work |= aDelegate->DoDelayedWork(&delayed_work_time_);

    if (did_work && delayed_work_time_.is_null())
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
    
    
    NS_DispatchToMainThread(mDoWorkEvent, NS_DISPATCH_NORMAL);
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
  if (!mDelayedWorkTimer) {
    mDelayedWorkTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
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

  base::TimeDelta delay = aDelayedTime - base::TimeTicks::Now();
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

#ifdef DEBUG
namespace {
MessagePump::Delegate* gFirstDelegate = nullptr;
}
#endif

void
MessagePumpForChildProcess::Run(MessagePump::Delegate* aDelegate)
{
  if (mFirstRun) {
#ifdef DEBUG
    NS_ASSERTION(aDelegate && gFirstDelegate == nullptr, "Huh?!");
    gFirstDelegate = aDelegate;
#endif
    mFirstRun = false;
    if (NS_FAILED(XRE_RunAppShell())) {
        NS_WARNING("Failed to run app shell?!");
    }
#ifdef DEBUG
    NS_ASSERTION(aDelegate && aDelegate == gFirstDelegate, "Huh?!");
    gFirstDelegate = nullptr;
#endif
    return;
  }

#ifdef DEBUG
  NS_ASSERTION(aDelegate && aDelegate == gFirstDelegate, "Huh?!");
#endif

  
  
  
  
  
  
  
  
  
  MessageLoop* loop = MessageLoop::current();
  bool nestableTasksAllowed = loop->NestableTasksAllowed();
  loop->SetNestableTasksAllowed(true);

  while (aDelegate->DoWork());

  loop->SetNestableTasksAllowed(nestableTasksAllowed);


  
  mozilla::ipc::MessagePump::Run(aDelegate);
}
