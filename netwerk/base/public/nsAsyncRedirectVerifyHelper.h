




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
#include "mozilla/Attributes.h"

class nsIChannel;






class nsAsyncRedirectVerifyHelper MOZ_FINAL : public nsIRunnable,
                                              public nsIAsyncVerifyRedirectCallback
{
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

public:
    nsAsyncRedirectVerifyHelper();

    



    nsresult DelegateOnChannelRedirect(nsIChannelEventSink *sink,
                                       nsIChannel *oldChannel, 
                                       nsIChannel *newChannel,
                                       uint32_t flags);
 
    















    nsresult Init(nsIChannel* oldChan,
                  nsIChannel* newChan,
                  uint32_t flags,
                  bool synchronize = false);

protected:
    nsCOMPtr<nsIChannel> mOldChan;
    nsCOMPtr<nsIChannel> mNewChan;
    uint32_t mFlags;
    bool mWaitingForRedirectCallback;
    nsCOMPtr<nsIThread>      mCallbackThread;
    bool                     mCallbackInitiated;
    int32_t                  mExpectedCallbacks;
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
    explicit nsAsyncRedirectAutoCallback(nsIAsyncVerifyRedirectCallback* aCallback)
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
        mCallback = nullptr;
    }
private:
    nsIAsyncVerifyRedirectCallback* mCallback;
    nsresult mResult;
};

#endif
