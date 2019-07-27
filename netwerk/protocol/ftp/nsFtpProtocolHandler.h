




#ifndef nsFtpProtocolHandler_h__
#define nsFtpProtocolHandler_h__

#include "nsFtpControlConnection.h"
#include "nsIProxiedProtocolHandler.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"



class nsFtpProtocolHandler MOZ_FINAL : public nsIProxiedProtocolHandler
                                     , public nsIObserver
                                     , public nsSupportsWeakReference
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIPROXIEDPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER
    
    nsFtpProtocolHandler();
    
    nsresult Init();

    
    nsresult InsertConnection(nsIURI *aKey, nsFtpControlConnection *aConn);
    nsresult RemoveConnection(nsIURI *aKey, nsFtpControlConnection **aConn);
    uint32_t GetSessionId() { return mSessionId; }

    uint8_t GetDataQoSBits() { return mDataQoSBits; }
    uint8_t GetControlQoSBits() { return mControlQoSBits; }

private:
    virtual ~nsFtpProtocolHandler();

    
    struct timerStruct {
        nsCOMPtr<nsITimer>      timer;
        nsFtpControlConnection *conn;
        char                   *key;
        
        timerStruct() : conn(nullptr), key(nullptr) {}
        
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
    void ClearAllConnections();

    nsTArray<timerStruct*> mRootConnectionList;

    int32_t mIdleTimeout;

    
    
    
    
    
    uint32_t mSessionId;

    uint8_t mControlQoSBits;
    uint8_t mDataQoSBits;
};



extern nsFtpProtocolHandler *gFtpHandler;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gFTPLog;
#endif

#endif 
