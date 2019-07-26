



#ifndef nsBrowserStatusFilter_h__
#define nsBrowserStatusFilter_h__

#include "nsIWebProgressListener.h"
#include "nsIWebProgressListener2.h"
#include "nsIWebProgress.h"
#include "nsWeakReference.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsString.h"







class nsBrowserStatusFilter : public nsIWebProgress
                            , public nsIWebProgressListener2
                            , public nsSupportsWeakReference
{
public:
    nsBrowserStatusFilter();
    virtual ~nsBrowserStatusFilter();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIWEBPROGRESSLISTENER2

private:
    nsresult StartDelayTimer();
    void ProcessTimeout();
    void MaybeSendProgress();
    void MaybeSendStatus();
    void ResetMembers();
    bool DelayInEffect() { return mDelayedStatus || mDelayedProgress; }

    static void TimeoutHandler(nsITimer *aTimer, void *aClosure);

private:
    nsCOMPtr<nsIWebProgressListener> mListener;
    nsCOMPtr<nsITimer>               mTimer;

    
    nsString                         mStatusMsg;
    int64_t                          mCurProgress;
    int64_t                          mMaxProgress;

    nsString                         mCurrentStatusMsg;
    bool                             mStatusIsDirty;
    int32_t                          mCurrentPercentage;

    
    int32_t                          mTotalRequests;
    int32_t                          mFinishedRequests;
    bool                             mUseRealProgressFlag;

    
    bool                             mDelayedStatus;
    bool                             mDelayedProgress;
};

#define NS_BROWSERSTATUSFILTER_CONTRACTID \
    "@mozilla.org/appshell/component/browser-status-filter;1"
#define NS_BROWSERSTATUSFILTER_CID                   \
{ /* 6356aa16-7916-4215-a825-cbc2692ca87a */         \
    0x6356aa16,                                      \
    0x7916,                                          \
    0x4215,                                          \
    {0xa8, 0x25, 0xcb, 0xc2, 0x69, 0x2c, 0xa8, 0x7a} \
}

#endif 
