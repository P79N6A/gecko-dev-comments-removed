




#ifndef NSNOTIFYADDRLISTENER_LINUX_H_
#define NSNOTIFYADDRLISTENER_LINUX_H_

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "nsINetworkLinkService.h"
#include "nsIRunnable.h"
#include "nsIObserver.h"
#include "nsThreadUtils.h"
#include "nsCOMPtr.h"
#include "mozilla/TimeStamp.h"

class nsNotifyAddrListener : public nsINetworkLinkService,
                             public nsIRunnable,
                             public nsIObserver
{
    virtual ~nsNotifyAddrListener();

public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSINETWORKLINKSERVICE
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSIOBSERVER

    nsNotifyAddrListener();
    nsresult Init(void);

private:
    class ChangeEvent : public nsRunnable {
    public:
        NS_DECL_NSIRUNNABLE
        ChangeEvent(nsINetworkLinkService *aService, const char *aEventID)
            : mService(aService), mEventID(aEventID) {
        }
    private:
        nsCOMPtr<nsINetworkLinkService> mService;
        const char *mEventID;
    };

    
    nsresult Shutdown(void);

    
    nsresult SendEvent(const char *aEventID);

    
    void checkLink(void);

    
    void OnNetlinkMessage(int NetlinkSocket);

    nsCOMPtr<nsIThread> mThread;

    
    bool mLinkUp;

    
    bool mStatusKnown;

    
    int mShutdownPipe[2];

    
    bool mAllowChangedEvent;

    
    bool mChildThreadShutdown;
};

#endif 
