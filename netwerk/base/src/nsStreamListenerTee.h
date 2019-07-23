




































#ifndef nsStreamListenerTee_h__
#define nsStreamListenerTee_h__

#include "nsIStreamListenerTee.h"
#include "nsIInputStreamTee.h"
#include "nsIOutputStream.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"

class nsStreamListenerTee : public nsIStreamListenerTee
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMLISTENERTEE

    nsStreamListenerTee() { }
    virtual ~nsStreamListenerTee() { }

private:
    nsCOMPtr<nsIInputStreamTee>  mInputTee;
    nsCOMPtr<nsIOutputStream>    mSink;
    nsCOMPtr<nsIStreamListener>  mListener;
    nsCOMPtr<nsIRequestObserver> mObserver;
    nsCOMPtr<nsIEventTarget>     mEventTarget;
};

#endif
