




#ifndef nsRequestObserverProxy_h__
#define nsRequestObserverProxy_h__

#include "nsIRequestObserver.h"
#include "nsIRequestObserverProxy.h"
#include "nsIRequest.h"
#include "nsThreadUtils.h"
#include "nsCOMPtr.h"
#include "nsProxyRelease.h"

class nsARequestObserverEvent;

class nsRequestObserverProxy MOZ_FINAL : public nsIRequestObserverProxy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIREQUESTOBSERVERPROXY

    nsRequestObserverProxy() {}

    nsIRequestObserver *Observer() { return mObserver; }

    nsresult FireEvent(nsARequestObserverEvent *);

protected:
    nsMainThreadPtrHandle<nsIRequestObserver> mObserver;
    nsMainThreadPtrHandle<nsISupports>        mContext;

    friend class nsOnStartRequestEvent;
    friend class nsOnStopRequestEvent;
};

class nsARequestObserverEvent : public nsRunnable
{
public:
    nsARequestObserverEvent(nsIRequest *);

protected:
    virtual ~nsARequestObserverEvent() {}

    nsCOMPtr<nsIRequest>  mRequest;
};

#endif 
