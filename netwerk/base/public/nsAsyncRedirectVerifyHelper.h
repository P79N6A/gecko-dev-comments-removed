






































#ifndef nsAsyncRedirectVerifyHelper_h
#define nsAsyncRedirectVerifyHelper_h

#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"

class nsIChannel;






class nsAsyncRedirectVerifyHelper : public nsIRunnable,
                                    public nsIAsyncVerifyRedirectCallback
{
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

public:
    nsAsyncRedirectVerifyHelper();

    



    nsresult DelegateOnChannelRedirect(nsIChannelEventSink *sink,
                                       nsIChannel *oldChannel, 
                                       nsIChannel *newChannel,
                                       PRUint32 flags);
 
    















    nsresult Init(nsIChannel* oldChan,
                  nsIChannel* newChan,
                  PRUint32 flags,
                  bool synchronize = false);

protected:
    nsCOMPtr<nsIChannel> mOldChan;
    nsCOMPtr<nsIChannel> mNewChan;
    PRUint32 mFlags;
    bool mWaitingForRedirectCallback;
    nsCOMPtr<nsIThread>      mCallbackThread;
    bool                     mCallbackInitiated;
    PRInt32                  mExpectedCallbacks;
    nsresult                 mResult; 

    void InitCallback();
    
    


    void ExplicitCallback(nsresult result);

private:
    ~nsAsyncRedirectVerifyHelper();
    
    bool IsOldChannelCanceled();
};




class nsAsyncRedirectAutoCallback
{
public:
    nsAsyncRedirectAutoCallback(nsIAsyncVerifyRedirectCallback* aCallback)
        : mCallback(aCallback)
    {
        mResult = NS_OK;
    }
    ~nsAsyncRedirectAutoCallback()
    {
        if (mCallback)
            mCallback->OnRedirectVerifyCallback(mResult);
    }
    


    void SetResult(nsresult aRes)
    {
        mResult = aRes;
    }
    


    void DontCallback()
    {
        mCallback = nsnull;
    }
private:
    nsIAsyncVerifyRedirectCallback* mCallback;
    nsresult mResult;
};

#endif
