



#ifndef NSNETWORKLINKSERVICEMAC_H_
#define NSNETWORKLINKSERVICEMAC_H_

#include "nsINetworkLinkService.h"
#include "nsIObserver.h"

#include <SystemConfiguration/SCNetworkReachability.h>
#include <SystemConfiguration/SystemConfiguration.h>

class nsNetworkLinkService : public nsINetworkLinkService,
                             public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSINETWORKLINKSERVICE
    NS_DECL_NSIOBSERVER

    nsNetworkLinkService();

    nsresult Init();
    nsresult Shutdown();

protected:
    virtual ~nsNetworkLinkService();

private:
    bool mLinkUp;
    bool mStatusKnown;

    
    bool mAllowChangedEvent;

    SCNetworkReachabilityRef mReachability;
    CFRunLoopRef mCFRunLoop;
    CFRunLoopSourceRef mRunLoopSource;
    SCDynamicStoreRef mStoreRef;

    void UpdateReachability();
    void SendEvent(bool aNetworkChanged);
    static void ReachabilityChanged(SCNetworkReachabilityRef target,
                                    SCNetworkConnectionFlags flags,
                                    void *info);
    static void IPConfigChanged(SCDynamicStoreRef store,
                                CFArrayRef changedKeys,
                                void *info);
};

#endif 
