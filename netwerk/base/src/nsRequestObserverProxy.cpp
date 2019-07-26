




#include "mozilla/DebugOnly.h"

#include "nscore.h"
#include "nsRequestObserverProxy.h"
#include "nsIRequest.h"
#include "nsIServiceManager.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "prlog.h"

using namespace mozilla;

#if defined(PR_LOGGING)
static PRLogModuleInfo *gRequestObserverProxyLog;
#endif

#define LOG(args) PR_LOG(gRequestObserverProxyLog, PR_LOG_DEBUG, args)





nsARequestObserverEvent::nsARequestObserverEvent(nsIRequest *request)
    : mRequest(request)
{
    NS_PRECONDITION(mRequest, "null pointer");
}





class nsOnStartRequestEvent : public nsARequestObserverEvent
{
    nsRefPtr<nsRequestObserverProxy> mProxy;
public:
    nsOnStartRequestEvent(nsRequestObserverProxy *proxy,
                          nsIRequest *request)
        : nsARequestObserverEvent(request)
        , mProxy(proxy)
    {
        NS_PRECONDITION(mProxy, "null pointer");
    }

    virtual ~nsOnStartRequestEvent() {}

    NS_IMETHOD Run()
    {
        LOG(("nsOnStartRequestEvent::HandleEvent [req=%x]\n", mRequest.get()));

        if (!mProxy->mObserver) {
            NS_NOTREACHED("already handled onStopRequest event (observer is null)");
            return NS_OK;
        }

        LOG(("handle startevent=%p\n", this));
        nsresult rv = mProxy->mObserver->OnStartRequest(mRequest, mProxy->mContext);
        if (NS_FAILED(rv)) {
            LOG(("OnStartRequest failed [rv=%x] canceling request!\n", rv));
            rv = mRequest->Cancel(rv);
            NS_ASSERTION(NS_SUCCEEDED(rv), "Cancel failed for request!");
        }

        return NS_OK;
    }
};





class nsOnStopRequestEvent : public nsARequestObserverEvent
{
    nsRefPtr<nsRequestObserverProxy> mProxy;
public:
    nsOnStopRequestEvent(nsRequestObserverProxy *proxy,
                         nsIRequest *request)
        : nsARequestObserverEvent(request)
        , mProxy(proxy)
    {
        NS_PRECONDITION(mProxy, "null pointer");
    }

    virtual ~nsOnStopRequestEvent() {}

    NS_IMETHOD Run()
    {
        LOG(("nsOnStopRequestEvent::HandleEvent [req=%x]\n", mRequest.get()));

        nsMainThreadPtrHandle<nsIRequestObserver> observer = mProxy->mObserver;
        if (!observer) {
            NS_NOTREACHED("already handled onStopRequest event (observer is null)");
            return NS_OK;
        }
        
        mProxy->mObserver = 0;

        nsresult status = NS_OK;
        DebugOnly<nsresult> rv = mRequest->GetStatus(&status);
        NS_ASSERTION(NS_SUCCEEDED(rv), "GetStatus failed for request!");

        LOG(("handle stopevent=%p\n", this));
        (void) observer->OnStopRequest(mRequest, mProxy->mContext, status);

        return NS_OK;
    }
};





NS_IMPL_THREADSAFE_ISUPPORTS2(nsRequestObserverProxy,
                              nsIRequestObserver,
                              nsIRequestObserverProxy)





NS_IMETHODIMP 
nsRequestObserverProxy::OnStartRequest(nsIRequest *request,
                                       nsISupports *context)
{
    MOZ_ASSERT(!context || context == mContext);
    LOG(("nsRequestObserverProxy::OnStartRequest [this=%x req=%x]\n", this, request));

    nsOnStartRequestEvent *ev = 
        new nsOnStartRequestEvent(this, request);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    LOG(("post startevent=%p\n", ev));
    nsresult rv = FireEvent(ev);
    if (NS_FAILED(rv))
        delete ev;
    return rv;
}

NS_IMETHODIMP 
nsRequestObserverProxy::OnStopRequest(nsIRequest *request,
                                      nsISupports *context,
                                      nsresult status)
{
    MOZ_ASSERT(!context || context == mContext);
    LOG(("nsRequestObserverProxy: OnStopRequest [this=%x req=%x status=%x]\n",
        this, request, status));

    
    
    
    

    nsOnStopRequestEvent *ev = 
        new nsOnStopRequestEvent(this, request);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    LOG(("post stopevent=%p\n", ev));
    nsresult rv = FireEvent(ev);
    if (NS_FAILED(rv))
        delete ev;
    return rv;
}





NS_IMETHODIMP
nsRequestObserverProxy::Init(nsIRequestObserver *observer, nsISupports *context)
{
    NS_ENSURE_ARG_POINTER(observer);

#if defined(PR_LOGGING)
    if (!gRequestObserverProxyLog)
        gRequestObserverProxyLog = PR_NewLogModule("nsRequestObserverProxy");
#endif

    mObserver = new nsMainThreadPtrHolder<nsIRequestObserver>(observer);
    mContext = new nsMainThreadPtrHolder<nsISupports>(context);

    return NS_OK;
}





nsresult
nsRequestObserverProxy::FireEvent(nsARequestObserverEvent *event)
{
    nsCOMPtr<nsIEventTarget> mainThread(do_GetMainThread());
    return mainThread->Dispatch(event, NS_DISPATCH_NORMAL);
}
