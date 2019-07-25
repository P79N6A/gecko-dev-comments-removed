




































#ifndef NSNETWORKLINKSERVICEMAC_H_
#define NSNETWORKLINKSERVICEMAC_H_

#include "nsINetworkLinkService.h"
#include "nsIObserver.h"

#include <SystemConfiguration/SCNetworkReachability.h>

class nsNetworkLinkService : public nsINetworkLinkService,
                             public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSINETWORKLINKSERVICE
    NS_DECL_NSIOBSERVER

    nsNetworkLinkService();
    virtual ~nsNetworkLinkService();

    nsresult Init();
    nsresult Shutdown();

private:
    bool mLinkUp;
    bool mStatusKnown;

    SCNetworkReachabilityRef mReachability;
    CFRunLoopRef mCFRunLoop;

    void UpdateReachability();
    void SendEvent();
    static void ReachabilityChanged(SCNetworkReachabilityRef target,
                                    SCNetworkConnectionFlags flags,
                                    void *info);
};

#endif 
