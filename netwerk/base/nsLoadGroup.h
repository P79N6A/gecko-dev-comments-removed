




#ifndef nsLoadGroup_h__
#define nsLoadGroup_h__

#include "nsILoadGroup.h"
#include "nsILoadGroupChild.h"
#include "nsPILoadGroupInternal.h"
#include "nsAgg.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"
#include "nsISupportsPriority.h"
#include "pldhash.h"
#include "mozilla/TimeStamp.h"

class nsILoadGroupConnectionInfo;
class nsITimedChannel;

class nsLoadGroup : public nsILoadGroup,
                    public nsILoadGroupChild,
                    public nsISupportsPriority,
                    public nsSupportsWeakReference,
                    public nsPILoadGroupInternal
{
public:
    NS_DECL_AGGREGATED
    
    
    
    NS_DECL_NSIREQUEST

    
    
    NS_DECL_NSILOADGROUP
    NS_DECL_NSPILOADGROUPINTERNAL

    
    
    NS_DECL_NSILOADGROUPCHILD

    
    
    NS_DECL_NSISUPPORTSPRIORITY

    
    

    explicit nsLoadGroup(nsISupports* outer);

    nsresult Init();

protected:
    virtual ~nsLoadGroup();

    nsresult MergeLoadFlags(nsIRequest *aRequest, nsLoadFlags& flags);

private:
    void TelemetryReport();
    void TelemetryReportChannel(nsITimedChannel *timedChannel,
                                bool defaultRequest);

protected:
    uint32_t                        mForegroundCount;
    uint32_t                        mLoadFlags;
    uint32_t                        mDefaultLoadFlags;

    nsCOMPtr<nsILoadGroup>          mLoadGroup; 
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsILoadGroupConnectionInfo> mConnectionInfo;

    nsCOMPtr<nsIRequest>            mDefaultLoadRequest;
    PLDHashTable                    mRequests;

    nsWeakPtr                       mObserver;
    nsWeakPtr                       mParentLoadGroup;
    
    nsresult                        mStatus;
    int32_t                         mPriority;
    bool                            mIsCanceling;

    
    mozilla::TimeStamp              mDefaultRequestCreationTime;
    bool                            mDefaultLoadIsTimed;
    uint32_t                        mTimedRequests;
    uint32_t                        mCachedRequests;

    
    uint32_t                        mTimedNonCachedRequestsUntilOnEndPageLoad;
};

#endif 
