






































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "prlog.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"

#include "nsIOService.h"
#include "nsIChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIAsyncVerifyRedirectCallback.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *gLog = PR_NewLogModule("nsRedirect");
#define LOG(args) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

NS_IMPL_THREADSAFE_ISUPPORTS2(nsAsyncRedirectVerifyHelper,
                              nsIAsyncVerifyRedirectCallback,
                              nsIRunnable)

class nsAsyncVerifyRedirectCallbackEvent : public nsRunnable {
public:
    nsAsyncVerifyRedirectCallbackEvent(nsIAsyncVerifyRedirectCallback *cb,
                                       nsresult result)
        : mCallback(cb), mResult(result) {
    }

    NS_IMETHOD Run()
    {
        LOG(("nsAsyncVerifyRedirectCallbackEvent::Run() "
             "callback to %p with result %x",
             mCallback.get(), mResult));
       (void) mCallback->OnRedirectVerifyCallback(mResult);
       return NS_OK;
    }
private:
    nsCOMPtr<nsIAsyncVerifyRedirectCallback> mCallback;
    nsresult mResult;
};

nsAsyncRedirectVerifyHelper::nsAsyncRedirectVerifyHelper()
    : mCallbackInitiated(PR_FALSE),
      mExpectedCallbacks(0),
      mResult(NS_OK)
{
}

nsAsyncRedirectVerifyHelper::~nsAsyncRedirectVerifyHelper()
{
    NS_ASSERTION(NS_FAILED(mResult) || mExpectedCallbacks == 0,
                 "Did not receive all required callbacks!");
}

nsresult
nsAsyncRedirectVerifyHelper::Init(nsIChannel* oldChan, nsIChannel* newChan,
                                  PRUint32 flags, bool synchronize)
{
    LOG(("nsAsyncRedirectVerifyHelper::Init() "
         "oldChan=%p newChan=%p", oldChan, newChan));
    mOldChan           = oldChan;
    mNewChan           = newChan;
    mFlags             = flags;
    mCallbackThread    = do_GetCurrentThread();

    if (synchronize)
      mWaitingForRedirectCallback = PR_TRUE;

    nsresult rv;
    rv = NS_DispatchToMainThread(this);
    NS_ENSURE_SUCCESS(rv, rv);

    if (synchronize) {
      nsIThread *thread = NS_GetCurrentThread();
      while (mWaitingForRedirectCallback) {
        if (!NS_ProcessNextEvent(thread)) {
          return NS_ERROR_UNEXPECTED;
        }
      }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsAsyncRedirectVerifyHelper::OnRedirectVerifyCallback(nsresult result)
{
    LOG(("nsAsyncRedirectVerifyHelper::OnRedirectVerifyCallback() "
         "result=%x expectedCBs=%u mResult=%x",
         result, mExpectedCallbacks, mResult));

    --mExpectedCallbacks;

    
    if (NS_FAILED(result)) {
        
        if (NS_SUCCEEDED(mResult))
            mResult = result;

        
        
        if (mCallbackInitiated) {
            ExplicitCallback(mResult);
            return NS_OK;
        }
    }

    
    
    if (mCallbackInitiated && mExpectedCallbacks == 0) {
        ExplicitCallback(mResult);
    }

    return NS_OK;
}

nsresult
nsAsyncRedirectVerifyHelper::DelegateOnChannelRedirect(nsIChannelEventSink *sink,
                                                       nsIChannel *oldChannel,
                                                       nsIChannel *newChannel,
                                                       PRUint32 flags)
{
    LOG(("nsAsyncRedirectVerifyHelper::DelegateOnChannelRedirect() "
         "sink=%p expectedCBs=%u mResult=%x",
         sink, mExpectedCallbacks, mResult));

    ++mExpectedCallbacks;

    if (IsOldChannelCanceled()) {
        LOG(("  old channel has been canceled, cancel the redirect by "
             "emulating OnRedirectVerifyCallback..."));
        (void) OnRedirectVerifyCallback(NS_BINDING_ABORTED);
        return NS_BINDING_ABORTED;
    }

    nsresult rv =
        sink->AsyncOnChannelRedirect(oldChannel, newChannel, flags, this);

    LOG(("  result=%x expectedCBs=%u", rv, mExpectedCallbacks));

    
    
    
    if (NS_FAILED(rv)) {
        LOG(("  emulating OnRedirectVerifyCallback..."));
        (void) OnRedirectVerifyCallback(rv);
    }

    return rv;  
}

void
nsAsyncRedirectVerifyHelper::ExplicitCallback(nsresult result)
{
    LOG(("nsAsyncRedirectVerifyHelper::ExplicitCallback() "
         "result=%x expectedCBs=%u mCallbackInitiated=%u mResult=%x",
         result, mExpectedCallbacks, mCallbackInitiated, mResult));

    nsCOMPtr<nsIAsyncVerifyRedirectCallback>
        callback(do_QueryInterface(mOldChan));

    if (!callback || !mCallbackThread) {
        LOG(("nsAsyncRedirectVerifyHelper::ExplicitCallback() "
             "callback=%p mCallbackThread=%p", callback.get(), mCallbackThread.get()));
        return;
    }

    mCallbackInitiated = PR_FALSE;  
    mWaitingForRedirectCallback = PR_FALSE;

    
    nsRefPtr<nsIRunnable> event =
        new nsAsyncVerifyRedirectCallbackEvent(callback, result);
    if (!event) {
        NS_WARNING("nsAsyncRedirectVerifyHelper::ExplicitCallback() "
                   "failed creating callback event!");
        return;
    }
    nsresult rv = mCallbackThread->Dispatch(event, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
        NS_WARNING("nsAsyncRedirectVerifyHelper::ExplicitCallback() "
                   "failed dispatching callback event!");
    } else {
        LOG(("nsAsyncRedirectVerifyHelper::ExplicitCallback() "
             "dispatched callback event=%p", event.get()));
    }
   
}

void
nsAsyncRedirectVerifyHelper::InitCallback()
{
    LOG(("nsAsyncRedirectVerifyHelper::InitCallback() "
         "expectedCBs=%d mResult=%x", mExpectedCallbacks, mResult));

    mCallbackInitiated = PR_TRUE;

    
    if (mExpectedCallbacks == 0)
        ExplicitCallback(mResult);
}

NS_IMETHODIMP
nsAsyncRedirectVerifyHelper::Run()
{
    




    if (IsOldChannelCanceled()) {
        ExplicitCallback(NS_BINDING_ABORTED);
        return NS_OK;
    }

    
    NS_ASSERTION(gIOService, "Must have an IO service at this point");
    LOG(("nsAsyncRedirectVerifyHelper::Run() calling gIOService..."));
    nsresult rv = gIOService->AsyncOnChannelRedirect(mOldChan, mNewChan,
                                                     mFlags, this);
    if (NS_FAILED(rv)) {
        ExplicitCallback(rv);
        return NS_OK;
    }

    
    nsCOMPtr<nsIChannelEventSink> sink;
    NS_QueryNotificationCallbacks(mOldChan, sink);
    if (sink) {
        LOG(("nsAsyncRedirectVerifyHelper::Run() calling sink..."));
        rv = DelegateOnChannelRedirect(sink, mOldChan, mNewChan, mFlags);
    }

    
    
    InitCallback();
    return NS_OK;
}

bool
nsAsyncRedirectVerifyHelper::IsOldChannelCanceled()
{
    bool canceled;
    nsCOMPtr<nsIHttpChannelInternal> oldChannelInternal =
        do_QueryInterface(mOldChan);
    if (oldChannelInternal) {
        oldChannelInternal->GetCanceled(&canceled);
        if (canceled)
            return true;
    }

    return false;
}
