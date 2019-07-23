




































#ifndef NSNOTIFYADDRLISTENER_H_
#define NSNOTIFYADDRLISTENER_H_

#include <windows.h>
#include "nsINetworkLinkService.h"
#include "nsIRunnable.h"
#include "nsIObserver.h"
#include "nsThreadUtils.h"
#include "nsCOMPtr.h"

class nsNotifyAddrListener : public nsINetworkLinkService,
                             public nsIRunnable,
                             public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSINETWORKLINKSERVICE
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSIOBSERVER

    nsNotifyAddrListener();
    virtual ~nsNotifyAddrListener();

    nsresult Init(void);

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

    PRPackedBool mLinkUp;
    PRPackedBool mStatusKnown;

    nsresult Shutdown(void);
    nsresult SendEventToUI(const char *aEventID);

    DWORD GetOperationalStatus(DWORD aAdapterIndex);
    DWORD CheckIPAddrTable(void);
    DWORD CheckAdaptersInfo(void);
    DWORD CheckAdaptersAddresses(void);
    void  CheckLinkStatus(void);

    nsCOMPtr<nsIThread> mThread;

    OSVERSIONINFO mOSVerInfo;
    HANDLE        mShutdownEvent;
};

#endif 
