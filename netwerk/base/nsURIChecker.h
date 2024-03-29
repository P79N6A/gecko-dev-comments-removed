




#ifndef nsURIChecker_h__
#define nsURIChecker_h__

#include "nsIURIChecker.h"
#include "nsIChannel.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"



class nsURIChecker : public nsIURIChecker,
                     public nsIStreamListener,
                     public nsIChannelEventSink,
                     public nsIInterfaceRequestor
{
    virtual ~nsURIChecker() {}

public:
    nsURIChecker();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIURICHECKER
    NS_DECL_NSIREQUEST
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR

protected:
    nsCOMPtr<nsIChannel>         mChannel;
    nsCOMPtr<nsIRequestObserver> mObserver;
    nsCOMPtr<nsISupports>        mObserverContext;
    nsresult                     mStatus;
    bool                         mIsPending;
    bool                         mAllowHead;

    void     SetStatusAndCallBack(nsresult aStatus);
    nsresult CheckStatus();
};

#endif 
