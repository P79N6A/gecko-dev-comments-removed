




#ifndef NSNOTIFYADDRLISTENER_H_
#define NSNOTIFYADDRLISTENER_H_

#include <windows.h>
#include <winsock2.h>
#include <iptypes.h>
#include "nsINetworkLinkService.h"
#include "nsIRunnable.h"
#include "nsIObserver.h"
#include "nsThreadUtils.h"
#include "nsCOMPtr.h"

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
    void CheckLinkStatus(void);

protected:
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

    bool mLinkUp;
    bool mStatusKnown;
    bool mCheckAttempted;

    nsresult Shutdown(void);
    nsresult SendEvent(const char *aEventID);

    DWORD CheckAdaptersAddresses(void);

    
    bool  CheckICSGateway(PIP_ADAPTER_ADDRESSES aAdapter);
    bool  CheckICSStatus(PWCHAR aAdapterName);

    nsCOMPtr<nsIThread> mThread;

    HANDLE        mShutdownEvent;

private:
    
    
    ULONG mIPInterfaceChecksum;
};

#endif 
