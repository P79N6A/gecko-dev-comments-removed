



































#include "mozilla/Mutex.h"
#include "nsTransportUtils.h"
#include "nsITransport.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

using namespace mozilla;



class nsTransportStatusEvent;

class nsTransportEventSinkProxy : public nsITransportEventSink
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITRANSPORTEVENTSINK

    nsTransportEventSinkProxy(nsITransportEventSink *sink,
                              nsIEventTarget *target,
                              bool coalesceAll)
        : mSink(sink)
        , mTarget(target)
        , mLock("nsTransportEventSinkProxy.mLock")
        , mLastEvent(nsnull)
        , mCoalesceAll(coalesceAll)
    {
        NS_ADDREF(mSink);
    }

    virtual ~nsTransportEventSinkProxy()
    {
        
        
        NS_ProxyRelease(mTarget, mSink);
    }

    nsITransportEventSink           *mSink;
    nsCOMPtr<nsIEventTarget>         mTarget;
    Mutex                            mLock;
    nsTransportStatusEvent          *mLastEvent;
    bool                             mCoalesceAll;
};

class nsTransportStatusEvent : public nsRunnable
{
public:
    nsTransportStatusEvent(nsTransportEventSinkProxy *proxy,
                           nsITransport *transport,
                           nsresult status,
                           PRUint64 progress,
                           PRUint64 progressMax)
        : mProxy(proxy)
        , mTransport(transport)
        , mStatus(status)
        , mProgress(progress)
        , mProgressMax(progressMax)
    {}

    ~nsTransportStatusEvent() {}

    NS_IMETHOD Run()
    {
        
        
        {
            MutexAutoLock lock(mProxy->mLock);
            if (mProxy->mLastEvent == this)
                mProxy->mLastEvent = nsnull;
        }

        mProxy->mSink->OnTransportStatus(mTransport, mStatus, mProgress,
                                         mProgressMax);
        return nsnull;
    }

    nsRefPtr<nsTransportEventSinkProxy> mProxy;

    
    nsCOMPtr<nsITransport> mTransport;
    nsresult               mStatus;
    PRUint64               mProgress;
    PRUint64               mProgressMax;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsTransportEventSinkProxy, nsITransportEventSink)

NS_IMETHODIMP
nsTransportEventSinkProxy::OnTransportStatus(nsITransport *transport,
                                             nsresult status,
                                             PRUint64 progress,
                                             PRUint64 progressMax)
{
    nsresult rv = NS_OK;
    nsRefPtr<nsTransportStatusEvent> event;
    {
        MutexAutoLock lock(mLock);

        
        if (mLastEvent && (mCoalesceAll || mLastEvent->mStatus == status)) {
            mLastEvent->mStatus = status;
            mLastEvent->mProgress = progress;
            mLastEvent->mProgressMax = progressMax;
        }
        else {
            event = new nsTransportStatusEvent(this, transport, status,
                                               progress, progressMax);
            if (!event)
                rv = NS_ERROR_OUT_OF_MEMORY;
            mLastEvent = event;  
        }
    }
    if (event) {
        rv = mTarget->Dispatch(event, NS_DISPATCH_NORMAL);
        if (NS_FAILED(rv)) {
            NS_WARNING("unable to post transport status event");

            MutexAutoLock lock(mLock); 
            mLastEvent = nsnull;
        }
    }
    return rv;
}



nsresult
net_NewTransportEventSinkProxy(nsITransportEventSink **result,
                               nsITransportEventSink *sink,
                               nsIEventTarget *target,
                               bool coalesceAll)
{
    *result = new nsTransportEventSinkProxy(sink, target, coalesceAll);
    if (!*result)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*result);
    return NS_OK;
}
