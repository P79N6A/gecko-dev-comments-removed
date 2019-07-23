




































#ifndef nsLoadGroup_h__
#define nsLoadGroup_h__

#include "nsILoadGroup.h"
#include "nsIChannel.h"
#include "nsIStreamListener.h"
#include "nsAgg.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsISupportsPriority.h"
#include "pldhash.h"

class  nsISupportsArray;

class nsLoadGroup : public nsILoadGroup,
                    public nsISupportsPriority,
                    public nsSupportsWeakReference
{
public:
    NS_DECL_AGGREGATED
    
    
    
    NS_DECL_NSIREQUEST

    
    
    NS_DECL_NSILOADGROUP

    
    
    NS_DECL_NSISUPPORTSPRIORITY

    
    

    nsLoadGroup(nsISupports* outer);

    nsresult Init();

protected:
    virtual ~nsLoadGroup();

    nsresult MergeLoadFlags(nsIRequest *aRequest, nsLoadFlags& flags);

protected:
    PRUint32                        mForegroundCount;
    PRUint32                        mLoadFlags;

    nsCOMPtr<nsILoadGroup>          mLoadGroup; 
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;

    nsCOMPtr<nsIRequest>            mDefaultLoadRequest;
    PLDHashTable                    mRequests;

    nsWeakPtr                       mObserver;
    
    nsresult                        mStatus;
    PRInt32                         mPriority;
    PRBool                          mIsCanceling;
};

#endif 
