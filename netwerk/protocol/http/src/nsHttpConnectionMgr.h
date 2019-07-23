





































#ifndef nsHttpConnectionMgr_h__
#define nsHttpConnectionMgr_h__

#include "nsHttpConnectionInfo.h"
#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "nsVoidArray.h"
#include "nsThreadUtils.h"
#include "nsHashtable.h"
#include "nsAutoPtr.h"
#include "prmon.h"

class nsHttpPipeline;



class nsHttpConnectionMgr
{
public:

    
    enum nsParamName {
        MAX_CONNECTIONS,
        MAX_CONNECTIONS_PER_HOST,
        MAX_CONNECTIONS_PER_PROXY,
        MAX_PERSISTENT_CONNECTIONS_PER_HOST,
        MAX_PERSISTENT_CONNECTIONS_PER_PROXY,
        MAX_REQUEST_DELAY,
        MAX_PIPELINED_REQUESTS
    };

    
    
    

    nsHttpConnectionMgr();

    nsresult Init(PRUint16 maxConnections,
                  PRUint16 maxConnectionsPerHost,
                  PRUint16 maxConnectionsPerProxy,
                  PRUint16 maxPersistentConnectionsPerHost,
                  PRUint16 maxPersistentConnectionsPerProxy,
                  PRUint16 maxRequestDelay,
                  PRUint16 maxPipelinedRequests);
    nsresult Shutdown();

    
    
    

    nsrefcnt AddRef()
    {
        return PR_AtomicIncrement(&mRef);
    }

    nsrefcnt Release()
    {
        nsrefcnt n = PR_AtomicDecrement(&mRef);
        if (n == 0)
            delete this;
        return n;
    }

    
    nsresult AddTransaction(nsHttpTransaction *, PRInt32 priority);

    
    
    nsresult RescheduleTransaction(nsHttpTransaction *, PRInt32 priority);

    
    nsresult CancelTransaction(nsHttpTransaction *, nsresult reason);

    
    
    nsresult PruneDeadConnections();

    
    
    nsresult GetSocketThreadTarget(nsIEventTarget **);

    
    
    
    nsresult ReclaimConnection(nsHttpConnection *conn);

    
    
    nsresult UpdateParam(nsParamName name, PRUint16 value);

    
    
    

    
    
    void AddTransactionToPipeline(nsHttpPipeline *);

    
    
    nsresult ProcessPendingQ(nsHttpConnectionInfo *);

private:
    virtual ~nsHttpConnectionMgr();

    
    
    
    
    
    
    struct nsConnectionEntry
    {
        nsConnectionEntry(nsHttpConnectionInfo *ci)
            : mConnInfo(ci)
        {
            NS_ADDREF(mConnInfo);
        }
       ~nsConnectionEntry() { NS_RELEASE(mConnInfo); }

        nsHttpConnectionInfo *mConnInfo;
        nsVoidArray           mPendingQ;    
        nsVoidArray           mActiveConns; 
        nsVoidArray           mIdleConns;   
    };

    
    
    
    
    
    
    
    
    
    class nsConnectionHandle : public nsAHttpConnection
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSAHTTPCONNECTION

        nsConnectionHandle(nsHttpConnection *conn) { NS_ADDREF(mConn = conn); }
        virtual ~nsConnectionHandle();

        nsHttpConnection *mConn;
    };

    
    
    

    PRInt32                      mRef;
    PRMonitor                   *mMonitor;
    nsCOMPtr<nsIEventTarget>     mSocketThreadTarget;

    
    PRUint16 mMaxConns;
    PRUint16 mMaxConnsPerHost;
    PRUint16 mMaxConnsPerProxy;
    PRUint16 mMaxPersistConnsPerHost;
    PRUint16 mMaxPersistConnsPerProxy;
    PRUint16 mMaxRequestDelay; 
    PRUint16 mMaxPipelinedRequests;

    
    
    

    static PRIntn PR_CALLBACK ProcessOneTransactionCB(nsHashKey *, void *, void *);
    static PRIntn PR_CALLBACK PurgeOneIdleConnectionCB(nsHashKey *, void *, void *);
    static PRIntn PR_CALLBACK PruneDeadConnectionsCB(nsHashKey *, void *, void *);
    static PRIntn PR_CALLBACK ShutdownPassCB(nsHashKey *, void *, void *);

    PRBool   ProcessPendingQForEntry(nsConnectionEntry *);
    PRBool   AtActiveConnectionLimit(nsConnectionEntry *, PRUint8 caps);
    void     GetConnection(nsConnectionEntry *, PRUint8 caps, nsHttpConnection **);
    nsresult DispatchTransaction(nsConnectionEntry *, nsAHttpTransaction *,
                                 PRUint8 caps, nsHttpConnection *);
    PRBool   BuildPipeline(nsConnectionEntry *, nsAHttpTransaction *, nsHttpPipeline **);
    nsresult ProcessNewTransaction(nsHttpTransaction *);

    
    typedef void (nsHttpConnectionMgr:: *nsConnEventHandler)(PRInt32, void *);

    
    
    
    
    
    class nsConnEvent;
    friend class nsConnEvent;
    class nsConnEvent : public nsRunnable
    {
    public:
        nsConnEvent(nsHttpConnectionMgr *mgr,
                    nsConnEventHandler handler,
                    PRInt32 iparam,
                    void *vparam)
            : mMgr(mgr)
            , mHandler(handler)
            , mIParam(iparam)
            , mVParam(vparam)
        {
            NS_ADDREF(mMgr);
        }

        NS_IMETHOD Run()
        {
            (mMgr->*mHandler)(mIParam, mVParam);
            return NS_OK;
        }

    private:
        virtual ~nsConnEvent()
        {
            NS_RELEASE(mMgr);
        }

        nsHttpConnectionMgr *mMgr;
        nsConnEventHandler   mHandler;
        PRInt32              mIParam;
        void                *mVParam;
    };

    nsresult PostEvent(nsConnEventHandler  handler,
                       PRInt32             iparam = 0,
                       void               *vparam = nsnull);

    
    void OnMsgShutdown             (PRInt32, void *);
    void OnMsgNewTransaction       (PRInt32, void *);
    void OnMsgReschedTransaction   (PRInt32, void *);
    void OnMsgCancelTransaction    (PRInt32, void *);
    void OnMsgProcessPendingQ      (PRInt32, void *);
    void OnMsgPruneDeadConnections (PRInt32, void *);
    void OnMsgReclaimConnection    (PRInt32, void *);
    void OnMsgUpdateParam          (PRInt32, void *);

    
    PRUint16 mNumActiveConns;
    PRUint16 mNumIdleConns;

    
    
    
    
    
    
    nsHashtable mCT;
};

#endif 
