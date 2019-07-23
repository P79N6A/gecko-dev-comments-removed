






































#include "nsAppShell.h"

#include "nsEventQueueWatcher.h"

#include "nsIEventQueueService.h"
#include "nsServiceManagerUtils.h"
#include "nsIEventQueue.h"

#include <qapplication.h>

nsAppShell::nsAppShell()
{
}

nsAppShell::~nsAppShell()
{
}




NS_IMPL_ISUPPORTS1(nsAppShell, nsIAppShell)

NS_IMETHODIMP
nsAppShell::Create(int *argc, char **argv)
{
    return NS_OK;
}

NS_IMETHODIMP
nsAppShell::Run(void)
{
    if (!mEventQueue)
        Spinup();

    if (!mEventQueue)
        return NS_ERROR_NOT_INITIALIZED;

    qApp->exec();

    Spindown();

    return NS_OK;
}



NS_IMETHODIMP
nsAppShell::Spinup()
{
    nsresult rv = NS_OK;

    
    nsCOMPtr<nsIEventQueueService> eventQService =
        do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID,&rv);

    if (NS_FAILED(rv)) {
        NS_WARNING("Could not obtain event queue service");
        return rv;
    }

    
    rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                                            getter_AddRefs(mEventQueue));

    
    if (mEventQueue)
        goto done;

    
    rv = eventQService->CreateThreadEventQueue();
    if (NS_FAILED(rv)) {
        NS_WARNING("Could not create the thread event queue");
        return rv;
    }

    
    rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD,getter_AddRefs(mEventQueue));
    if (NS_FAILED(rv)) {
        NS_ASSERTION("Could not obtain the thread event queue",PR_FALSE);
        return rv;
    }
done:
    AddEventQueue(mEventQueue);

    return NS_OK;
}

NS_IMETHODIMP
nsAppShell::Spindown(void)
{
    
    if (mEventQueue) {
        RemoveEventQueue(mEventQueue);
        mEventQueue->ProcessPendingEvents();
        mEventQueue = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsAppShell::ListenToEventQueue(nsIEventQueue *aQueue, PRBool aListen)
{
    
    
    

    if (aListen)
        AddEventQueue(aQueue);
    else
        RemoveEventQueue(aQueue);

    return NS_OK;
}

void
nsAppShell::AddEventQueue(nsIEventQueue *aQueue)
{
    nsEventQueueWatcher *que = 0;

    if ((que = mQueueDict.find(aQueue->GetEventQueueSelectFD()))) {
        que->ref();
    }
    else {
        mQueueDict.insert(aQueue->GetEventQueueSelectFD(),
                          new nsEventQueueWatcher(aQueue, qApp));
    }
}

void
nsAppShell::RemoveEventQueue(nsIEventQueue *aQueue)
{
    nsEventQueueWatcher *qtQueue = 0;

    if ((qtQueue = mQueueDict.find(aQueue->GetEventQueueSelectFD()))) {
        qtQueue->DataReceived();
        qtQueue->deref();
        if (qtQueue->count <= 0) {
            mQueueDict.take(aQueue->GetEventQueueSelectFD());
            delete qtQueue;
        }
    }
}

NS_IMETHODIMP
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void * &aEvent)
{
    aRealEvent = PR_FALSE;
    aEvent = 0;

    return NS_OK;
}

NS_IMETHODIMP
nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
    if (!mEventQueue)
        return NS_ERROR_NOT_INITIALIZED;

    qApp->processEvents();

    return NS_OK;
}

NS_IMETHODIMP
nsAppShell::Exit(void)
{
    qApp->exit(0);
    return NS_OK;
}
