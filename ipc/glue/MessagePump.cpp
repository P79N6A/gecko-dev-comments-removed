



































#include "MessagePump.h"

#include "nsIAppShell.h"
#include "nsIThread.h"
#include "nsIThreadInternal.h"

#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"
#include "nsWidgetsCID.h"
#include "prthread.h"

#include "base/logging.h"
#include "base/scoped_nsautorelease_pool.h"

using mozilla::ipc::MessagePump;
using mozilla::ipc::MessagePumpForChildProcess;

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

namespace mozilla {
namespace ipc {

class UIThreadObserver : public nsIThreadObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITHREADOBSERVER

  UIThreadObserver(MessagePump& aPump,
                   nsIThreadObserver* aRealObserver)
  : mPump(aPump),
    mRealObserver(aRealObserver),
    mPRThread(PR_GetCurrentThread())
  {
    NS_ASSERTION(aRealObserver, "This should never be null!");
  }

private:
  MessagePump& mPump;
  nsIThreadObserver* mRealObserver;
  PRThread* mPRThread;
};

} 
} 

using mozilla::ipc::UIThreadObserver;

NS_IMETHODIMP_(nsrefcnt)
UIThreadObserver::AddRef()
{
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
UIThreadObserver::Release()
{
  return 1;
}

NS_IMPL_QUERY_INTERFACE1(UIThreadObserver, nsIThreadObserver)

NS_IMETHODIMP
UIThreadObserver::OnDispatchedEvent(nsIThreadInternal* aThread)
{
  
  
  
  if (PR_GetCurrentThread() != mPRThread) {
    mPump.event_.Signal();
  }
  return mRealObserver->OnDispatchedEvent(aThread);
}

NS_IMETHODIMP
UIThreadObserver::OnProcessNextEvent(nsIThreadInternal* aThread,
                                     PRBool aMayWait,
                                     PRUint32 aRecursionDepth)
{
  return mRealObserver->OnProcessNextEvent(aThread, aMayWait, aRecursionDepth);
}

NS_IMETHODIMP
UIThreadObserver::AfterProcessNextEvent(nsIThreadInternal* aThread,
                                        PRUint32 aRecursionDepth)
{
  return mRealObserver->AfterProcessNextEvent(aThread, aRecursionDepth);
}

void
MessagePump::Run(MessagePump::Delegate* aDelegate)
{
  NS_ASSERTION(keep_running_, "Quit must have been called outside of Run!");

  nsCOMPtr<nsIThread> thread(do_GetCurrentThread());
  NS_ASSERTION(thread, "This should never be null!");

  nsCOMPtr<nsIThreadInternal> threadInternal(do_QueryInterface(thread));
  NS_ASSERTION(threadInternal, "QI failed?!");

  nsCOMPtr<nsIThreadObserver> realObserver;
  threadInternal->GetObserver(getter_AddRefs(realObserver));
  NS_ASSERTION(realObserver, "This should never be null!");

  UIThreadObserver observer(*this, realObserver);
  threadInternal->SetObserver(&observer);

#ifdef DEBUG
  {
    nsCOMPtr<nsIAppShell> appShell(do_QueryInterface(realObserver));
    NS_ASSERTION(appShell, "Should be the app shell!");
  }
#endif


  for (;;) {
    
    
    base::ScopedNSAutoreleasePool autorelease_pool;

    bool did_work = NS_ProcessNextEvent(thread, PR_FALSE) ? true : false;
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
      event_.Wait();
    } else {
      base::TimeDelta delay = delayed_work_time_ - base::Time::Now();
      if (delay > base::TimeDelta()) {
        event_.TimedWait(delay);
      } else {
        
        
        delayed_work_time_ = base::Time();
      }
    }
    
    
  }

  threadInternal->SetObserver(realObserver);

  keep_running_ = true;
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
    nsCOMPtr<nsIAppShell> appShell(do_GetService(kAppShellCID));
    NS_WARN_IF_FALSE(appShell, "Failed to get app shell?!");
    if (appShell) {
      nsresult rv = appShell->Run();
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to run app shell?!");
      }
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
