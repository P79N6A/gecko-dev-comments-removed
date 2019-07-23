



































#include "MessagePump.h"

#include "nsIThread.h"
#include "nsITimer.h"

#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "pratom.h"
#include "prthread.h"

#include "base/logging.h"
#include "base/scoped_nsautorelease_pool.h"

using mozilla::ipc::MessagePump;
using mozilla::ipc::MessagePumpForChildProcess;

namespace {

void
TimerCallback(nsITimer* aTimer,
              void* aClosure)
{
  MessagePump* messagePump = reinterpret_cast<MessagePump*>(aClosure);
  messagePump->ScheduleWork();
}

} 

class DoWorkRunnable : public nsRunnable
{
public:
  NS_IMETHOD Run() {
    MessageLoop* loop = MessageLoop::current();
    NS_ASSERTION(loop, "Shouldn't be null!");
    if (loop) {
      loop->DoWork();
    }
    return NS_OK;
  }
};

MessagePump::MessagePump()
: mThread(nsnull)
{
  mDummyEvent = new DoWorkRunnable();
  
  NS_ADDREF(mDummyEvent);
}

MessagePump::~MessagePump()
{
  NS_RELEASE(mDummyEvent);
}

void
MessagePump::Run(MessagePump::Delegate* aDelegate)
{
  NS_ASSERTION(keep_running_, "Quit must have been called outside of Run!");

  NS_ASSERTION(NS_IsMainThread(),
               "This should only ever happen on Gecko's main thread!");

  mThread = NS_GetCurrentThread();
  NS_ASSERTION(mThread, "This should never be null!");

  nsCOMPtr<nsITimer> timer(do_CreateInstance(NS_TIMER_CONTRACTID));
  NS_ASSERTION(timer, "Failed to create timer!");

  base::ScopedNSAutoreleasePool autoReleasePool;

  for (;;) {
    autoReleasePool.Recycle();
    timer->Cancel();

    bool did_work = NS_ProcessNextEvent(mThread, PR_FALSE) ? true : false;
    if (!keep_running_)
      break;

    did_work |= aDelegate->DoWork();
    if (!keep_running_)
      break;

    did_work |= aDelegate->DoDelayedWork(&delayed_work_time_);
    if (!keep_running_)
      break;

    if (did_work)
      continue;

    did_work = aDelegate->DoIdleWork();
    if (!keep_running_)
      break;

    if (did_work)
      continue;

    if (delayed_work_time_.is_null()) {
      
      NS_ProcessNextEvent(mThread, PR_TRUE);
      continue;
    }

    base::TimeDelta delay = delayed_work_time_ - base::Time::Now();
    if (delay > base::TimeDelta()) {
      PRUint32 delayMS = PRUint32(delay.InMilliseconds());
      timer->InitWithFuncCallback(TimerCallback, this, delayMS,
                                  nsITimer::TYPE_ONE_SHOT);
      
      
      NS_ProcessNextEvent(mThread, PR_TRUE);
      continue;
    }

    
    
    delayed_work_time_ = base::Time();
  }

  timer->Cancel();

  keep_running_ = true;
}

void
MessagePump::ScheduleWork()
{
  
  if (mThread) {
    mThread->Dispatch(mDummyEvent, NS_DISPATCH_NORMAL);
  }
  else {
    
    
    NS_DispatchToMainThread(mDummyEvent, NS_DISPATCH_NORMAL);
  }
  event_.Signal();
}

#ifdef DEBUG
namespace {
MessagePump::Delegate* gFirstDelegate = nsnull;
}
#endif

void
MessagePumpForChildProcess::Run(MessagePump::Delegate* aDelegate)
{
  if (mFirstRun) {
#ifdef DEBUG
    NS_ASSERTION(aDelegate && gFirstDelegate == nsnull, "Huh?!");
    gFirstDelegate = aDelegate;
#endif
    mFirstRun = false;
    if (NS_FAILED(XRE_RunAppShell())) {
        NS_WARNING("Failed to run app shell?!");
    }
#ifdef DEBUG
    NS_ASSERTION(aDelegate && aDelegate == gFirstDelegate, "Huh?!");
    gFirstDelegate = nsnull;
#endif
    return;
  }

#ifdef DEBUG
  NS_ASSERTION(aDelegate && aDelegate == gFirstDelegate, "Huh?!");
#endif
  
  mozilla::ipc::MessagePump::Run(aDelegate);
}
