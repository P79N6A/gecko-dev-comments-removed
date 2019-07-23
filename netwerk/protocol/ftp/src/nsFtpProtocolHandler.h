





































#ifndef nsFtpProtocolHandler_h__
#define nsFtpProtocolHandler_h__

#include "nsFtpControlConnection.h"
#include "nsIServiceManager.h"
#include "nsIProxiedProtocolHandler.h"
#include "nsVoidArray.h"
#include "nsIIOService.h"
#include "nsITimer.h"
#include "nsIObserverService.h"
#include "nsICacheSession.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsCRT.h"

class nsITimer;
class nsIStreamListener;



class nsFtpProtocolHandler : public nsIProxiedProtocolHandler
                           , public nsIObserver
                           , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIPROXIEDPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER
    
    nsFtpProtocolHandler();
    virtual ~nsFtpProtocolHandler();
    
    nsresult Init();

    
    nsresult InsertConnection(nsIURI *aKey, nsFtpControlConnection *aConn);
    nsresult RemoveConnection(nsIURI *aKey, nsFtpControlConnection **aConn);

private:
    
    struct timerStruct {
        nsCOMPtr<nsITimer>      timer;
        nsFtpControlConnection *conn;
        char                   *key;
        
        timerStruct() : conn(nsnull), key(nsnull) {}
        
        ~timerStruct() {
            if (timer)
                timer->Cancel();
            if (key)
                nsMemory::Free(key);
            if (conn) {
                conn->Disconnect(NS_ERROR_ABORT);
                NS_RELEASE(conn);
            }
        }
    };

    static void Timeout(nsITimer *aTimer, void *aClosure);

    nsVoidArray mRootConnectionList;

    nsCOMPtr<nsICacheSession> mCacheSession;
    PRInt32 mIdleTimeout;
};



extern nsFtpProtocolHandler *gFtpHandler;

#endif 
