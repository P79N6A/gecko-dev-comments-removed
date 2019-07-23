





































#ifndef nsRequestObserverProxy_h__
#define nsRequestObserverProxy_h__

#include "nsIRequestObserver.h"
#include "nsIRequestObserverProxy.h"
#include "nsIRequest.h"
#include "nsThreadUtils.h"
#include "nsCOMPtr.h"

class nsARequestObserverEvent;

class nsRequestObserverProxy : public nsIRequestObserverProxy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIREQUESTOBSERVERPROXY

    nsRequestObserverProxy() {}

    nsIRequestObserver *Observer() { return mObserver; }

    nsresult FireEvent(nsARequestObserverEvent *);
    nsIEventTarget *Target() { return mTarget; } 
    void SetTarget(nsIEventTarget *target) { mTarget = target; }

protected:
    virtual ~nsRequestObserverProxy();

    nsCOMPtr<nsIRequestObserver> mObserver;
    nsCOMPtr<nsIEventTarget>     mTarget;

    friend class nsOnStartRequestEvent;
    friend class nsOnStopRequestEvent;
};

class nsARequestObserverEvent : public nsRunnable
{
public:
    nsARequestObserverEvent(nsIRequest *, nsISupports *);

protected:
    virtual ~nsARequestObserverEvent() {}

    nsCOMPtr<nsIRequest>  mRequest;
    nsCOMPtr<nsISupports> mContext;
};

#endif 
