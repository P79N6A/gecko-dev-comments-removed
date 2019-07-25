





































#include "nsBrowserStatusFilter.h"
#include "nsIChannel.h"
#include "nsITimer.h"
#include "nsIServiceManager.h"
#include "nsString.h"









nsBrowserStatusFilter::nsBrowserStatusFilter()
    : mCurProgress(0)
    , mMaxProgress(0)
    , mStatusIsDirty(PR_TRUE)
    , mCurrentPercentage(0)
    , mTotalRequests(0)
    , mFinishedRequests(0)
    , mUseRealProgressFlag(PR_FALSE)
    , mDelayedStatus(PR_FALSE)
    , mDelayedProgress(PR_FALSE)
{
}

nsBrowserStatusFilter::~nsBrowserStatusFilter()
{
    if (mTimer) {
        mTimer->Cancel();
    }
}





NS_IMPL_ISUPPORTS4(nsBrowserStatusFilter,
                   nsIWebProgress,
                   nsIWebProgressListener,
                   nsIWebProgressListener2,
                   nsISupportsWeakReference)





NS_IMETHODIMP
nsBrowserStatusFilter::AddProgressListener(nsIWebProgressListener *aListener,
                                           PRUint32 aNotifyMask)
{
    mListener = aListener;
    return NS_OK;
}

NS_IMETHODIMP
nsBrowserStatusFilter::RemoveProgressListener(nsIWebProgressListener *aListener)
{
    if (aListener == mListener)
        mListener = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsBrowserStatusFilter::GetDOMWindow(nsIDOMWindow **aResult)
{
    NS_NOTREACHED("nsBrowserStatusFilter::GetDOMWindow");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBrowserStatusFilter::GetIsLoadingDocument(PRBool *aIsLoadingDocument)
{
    NS_NOTREACHED("nsBrowserStatusFilter::GetIsLoadingDocument");
    return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
nsBrowserStatusFilter::OnStateChange(nsIWebProgress *aWebProgress,
                                     nsIRequest *aRequest,
                                     PRUint32 aStateFlags,
                                     nsresult aStatus)
{
    if (!mListener)
        return NS_OK;

    if (aStateFlags & STATE_START) {
        if (aStateFlags & STATE_IS_NETWORK) {
            ResetMembers();
        }
        if (aStateFlags & STATE_IS_REQUEST) {
            ++mTotalRequests;

            
            
            
            mUseRealProgressFlag = (mTotalRequests == 1);
        }
    }
    else if (aStateFlags & STATE_STOP) {
        if (aStateFlags & STATE_IS_REQUEST) {
            ++mFinishedRequests;
            
            
            
            if (!mUseRealProgressFlag && mTotalRequests)
                OnProgressChange(nsnull, nsnull, 0, 0,
                                 mFinishedRequests, mTotalRequests);
        }
    }
    else if (aStateFlags & STATE_TRANSFERRING) {
        if (aStateFlags & STATE_IS_REQUEST) {
            if (!mUseRealProgressFlag && mTotalRequests)
                return OnProgressChange(nsnull, nsnull, 0, 0,
                                        mFinishedRequests, mTotalRequests);
        }

        
        return NS_OK;
    } else {
        
        return NS_OK;
    }

    
    
    PRBool isLoadingDocument = PR_FALSE;
    if ((aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK ||
         (aStateFlags & nsIWebProgressListener::STATE_IS_REQUEST &&
          mFinishedRequests == mTotalRequests &&
          (aWebProgress->GetIsLoadingDocument(&isLoadingDocument),
           !isLoadingDocument)))) {
        if (mTimer && (aStateFlags & nsIWebProgressListener::STATE_STOP)) {
            mTimer->Cancel();
            ProcessTimeout();
        }

        return mListener->OnStateChange(aWebProgress, aRequest, aStateFlags,
                                        aStatus);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBrowserStatusFilter::OnProgressChange(nsIWebProgress *aWebProgress,
                                        nsIRequest *aRequest,
                                        PRInt32 aCurSelfProgress,
                                        PRInt32 aMaxSelfProgress,
                                        PRInt32 aCurTotalProgress,
                                        PRInt32 aMaxTotalProgress)
{
    if (!mListener)
        return NS_OK;

    if (!mUseRealProgressFlag && aRequest)
        return NS_OK;

    
    
    

    mCurProgress = (PRInt64)aCurTotalProgress;
    mMaxProgress = (PRInt64)aMaxTotalProgress;

    if (mDelayedProgress)
        return NS_OK;

    if (!mDelayedStatus) {
        MaybeSendProgress();
        StartDelayTimer();
    }

    mDelayedProgress = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
nsBrowserStatusFilter::OnLocationChange(nsIWebProgress *aWebProgress,
                                        nsIRequest *aRequest,
                                        nsIURI *aLocation)
{
    if (!mListener)
        return NS_OK;

    return mListener->OnLocationChange(aWebProgress, aRequest, aLocation);
}

NS_IMETHODIMP
nsBrowserStatusFilter::OnStatusChange(nsIWebProgress *aWebProgress,
                                      nsIRequest *aRequest,
                                      nsresult aStatus,
                                      const PRUnichar *aMessage)
{
    if (!mListener)
        return NS_OK;

    
    
    
    if (mStatusIsDirty || !mCurrentStatusMsg.Equals(aMessage)) {
        mStatusIsDirty = PR_TRUE;
        mStatusMsg = aMessage;
    }

    if (mDelayedStatus)
        return NS_OK;

    if (!mDelayedProgress) {
      MaybeSendStatus();
      StartDelayTimer();
    }

    mDelayedStatus = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
nsBrowserStatusFilter::OnSecurityChange(nsIWebProgress *aWebProgress,
                                        nsIRequest *aRequest,
                                        PRUint32 aState)
{
    if (!mListener)
        return NS_OK;

    return mListener->OnSecurityChange(aWebProgress, aRequest, aState);
}




NS_IMETHODIMP
nsBrowserStatusFilter::OnProgressChange64(nsIWebProgress *aWebProgress,
                                          nsIRequest *aRequest,
                                          PRInt64 aCurSelfProgress,
                                          PRInt64 aMaxSelfProgress,
                                          PRInt64 aCurTotalProgress,
                                          PRInt64 aMaxTotalProgress)
{
    
    return OnProgressChange(aWebProgress, aRequest,
                            (PRInt32)aCurSelfProgress,
                            (PRInt32)aMaxSelfProgress,
                            (PRInt32)aCurTotalProgress,
                            (PRInt32)aMaxTotalProgress);
}

NS_IMETHODIMP
nsBrowserStatusFilter::OnRefreshAttempted(nsIWebProgress *aWebProgress,
                                          nsIURI *aUri,
                                          PRInt32 aDelay,
                                          PRBool aSameUri,
                                          PRBool *allowRefresh)
{
    nsCOMPtr<nsIWebProgressListener2> listener =
        do_QueryInterface(mListener);
    if (!listener) {
        *allowRefresh = PR_TRUE;
        return NS_OK;
    }

    return listener->OnRefreshAttempted(aWebProgress, aUri, aDelay, aSameUri,
                                        allowRefresh);
}





void
nsBrowserStatusFilter::ResetMembers()
{
    mTotalRequests = 0;
    mFinishedRequests = 0;
    mUseRealProgressFlag = PR_FALSE;
    mMaxProgress = 0;
    mCurProgress = 0;
    mCurrentPercentage = 0;
    mStatusIsDirty = PR_TRUE;
}

void
nsBrowserStatusFilter::MaybeSendProgress() 
{
    if (mCurProgress > mMaxProgress || mCurProgress <= 0) 
        return;

    
    PRInt32 percentage = (PRInt32) double(mCurProgress) * 100 / mMaxProgress;

    
    if (percentage > (mCurrentPercentage + 3)) {
        mCurrentPercentage = percentage;
        
        mListener->OnProgressChange(nsnull, nsnull, 0, 0,
                                    (PRInt32)mCurProgress,
                                    (PRInt32)mMaxProgress);
    }
}

void
nsBrowserStatusFilter::MaybeSendStatus()
{
    if (mStatusIsDirty) {
        mListener->OnStatusChange(nsnull, nsnull, 0, mStatusMsg.get());
        mCurrentStatusMsg = mStatusMsg;
        mStatusIsDirty = PR_FALSE;
    }
}

nsresult
nsBrowserStatusFilter::StartDelayTimer()
{
    NS_ASSERTION(!DelayInEffect(), "delay should not be in effect");

    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mTimer)
        return NS_ERROR_FAILURE;

    return mTimer->InitWithFuncCallback(TimeoutHandler, this, 160, 
                                        nsITimer::TYPE_ONE_SHOT);
}

void
nsBrowserStatusFilter::ProcessTimeout()
{
    mTimer = nsnull;

    if (!mListener)
        return;

    if (mDelayedStatus) {
        mDelayedStatus = PR_FALSE;
        MaybeSendStatus();
    }

    if (mDelayedProgress) {
        mDelayedProgress = PR_FALSE;
        MaybeSendProgress();
    }
}

void
nsBrowserStatusFilter::TimeoutHandler(nsITimer *aTimer, void *aClosure)
{
    nsBrowserStatusFilter *self = reinterpret_cast<nsBrowserStatusFilter *>(aClosure);
    if (!self) {
        NS_ERROR("no self");
        return;
    }

    self->ProcessTimeout();
}
