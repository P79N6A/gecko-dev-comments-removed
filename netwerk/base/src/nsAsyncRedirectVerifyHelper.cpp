




































#include "nsAsyncRedirectVerifyHelper.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"

#include "nsIOService.h"
#include "nsIChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIAsyncVerifyRedirectCallback.h"

NS_IMPL_ISUPPORTS1(nsAsyncRedirectVerifyHelper, nsIRunnable)

nsresult
nsAsyncRedirectVerifyHelper::Init(nsIChannel* oldChan, nsIChannel* newChan,
                                  PRUint32 flags, PRBool synchronize)
{
    mOldChan = oldChan;
    mNewChan = newChan;
    mFlags = flags;

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

void
nsAsyncRedirectVerifyHelper::Callback(nsresult result)
{
    
    nsCOMPtr<nsIAsyncVerifyRedirectCallback> callback(do_QueryInterface(mOldChan));
    NS_ASSERTION(callback, "nsAsyncRedirectVerifyHelper: oldChannel doesn't"
                           " implement nsIAsyncVerifyRedirectCallback");

    if (callback)
        callback->OnRedirectVerifyCallback(result);

    mWaitingForRedirectCallback = PR_FALSE;
}

NS_IMETHODIMP
nsAsyncRedirectVerifyHelper::Run()
{
    






    PRBool canceled;
    nsCOMPtr<nsIHttpChannelInternal> oldChannelInternal =
        do_QueryInterface(mOldChan);
    if (oldChannelInternal) {
      oldChannelInternal->GetCanceled(&canceled);
      if (canceled) {
          Callback(NS_BINDING_ABORTED);
          return NS_OK;
      }
    }

    
    NS_ASSERTION(gIOService, "Must have an IO service at this point");
    nsresult rv = gIOService->OnChannelRedirect(mOldChan, mNewChan, mFlags);
    if (NS_FAILED(rv)) {
        Callback(rv);
        return NS_OK;
    }

    
    nsCOMPtr<nsIChannelEventSink> sink;
    NS_QueryNotificationCallbacks(mOldChan, sink);
    if (sink)
        rv = sink->OnChannelRedirect(mOldChan, mNewChan, mFlags);

    Callback(rv);
    return NS_OK;
}
