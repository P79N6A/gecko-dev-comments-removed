




































#ifndef nsAsyncRedirectVerifyHelper_h
#define nsAsyncRedirectVerifyHelper_h

#include "nsIRunnable.h"

#include "nsCOMPtr.h"

class nsIChannel;






class nsAsyncRedirectVerifyHelper : public nsIRunnable
{
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

public:
    















    nsresult Init(nsIChannel* oldChan,
                  nsIChannel* newChan,
                  PRUint32 flags,
                  PRBool synchronize = PR_FALSE);

protected:
    nsCOMPtr<nsIChannel> mOldChan;
    nsCOMPtr<nsIChannel> mNewChan;
    PRUint32 mFlags;
    PRBool mWaitingForRedirectCallback;

    void Callback(nsresult result);
};

#endif
