





































#ifndef nsHttpConnectionMgr_h__
#define nsHttpConnectionMgr_h__

#include "nsHttpConnectionInfo.h"
#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsAutoPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsISocketTransportService.h"
#include "nsIDNSListener.h"

#include "nsIObserver.h"
#include "nsITimer.h"

class nsHttpPipeline;



class nsHttpConnectionMgr : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    
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

    
    
    

    
    
    void PruneDeadConnectionsAfter(PRUint32 time);

    
    
    void ConditionallyStopPruneDeadConnectionsTimer();

    
    nsresult AddTransaction(nsHttpTransaction *, PRInt32 priority);

    
    
    nsresult RescheduleTransaction(nsHttpTransaction *, PRInt32 priority);

    
    nsresult CancelTransaction(nsHttpTransaction *, nsresult reason);

    
    
    nsresult PruneDeadConnections();

    
    
    nsresult ClosePersistentConnections();

    
    
    nsresult GetSocketThreadTarget(nsIEventTarget **);

    
    
    
    nsresult ReclaimConnection(nsHttpConnection *conn);

    
    
    nsresult UpdateParam(nsParamName name, PRUint16 value);

    
    
    

    
    
    void AddTransactionToPipeline(nsHttpPipeline *);

    
    
    nsresult ProcessPendingQ(nsHttpConnectionInfo *);

    
    
    
    nsresult CloseIdleConnection(nsHttpConnection *);

    
    
    
    void ReportSpdyConnection(nsHttpConnection *, bool usingSpdy);

private:
    virtual ~nsHttpConnectionMgr();
    class nsHalfOpenSocket;
    
    
    
    
    
    
    
    struct nsConnectionEntry
    {
        nsConnectionEntry(nsHttpConnectionInfo *ci)
          : mConnInfo(ci),
            mUsingSpdy(false),
            mTestedSpdy(false),
            mSpdyRedir(false),
            mDidDNS(false),
            mSpdyPreferred(false)
        {
            NS_ADDREF(mConnInfo);
        }
        ~nsConnectionEntry();

        nsHttpConnectionInfo        *mConnInfo;
        nsTArray<nsHttpTransaction*> mPendingQ;    
        nsTArray<nsHttpConnection*>  mActiveConns; 
        nsTArray<nsHttpConnection*>  mIdleConns;   
        nsTArray<nsHalfOpenSocket*>  mHalfOpens;

        
        
        
        
        
        
        
        nsCString mDottedDecimalAddress;

        bool mUsingSpdy;
        bool mTestedSpdy;
        bool mSpdyRedir;
        bool mDidDNS;
        bool mSpdyPreferred;
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

    
    

    class nsHalfOpenSocket : public nsIOutputStreamCallback,
                             public nsITransportEventSink,
                             public nsIInterfaceRequestor,
                             public nsITimerCallback,
                             public nsIDNSListener
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOUTPUTSTREAMCALLBACK
        NS_DECL_NSITRANSPORTEVENTSINK
        NS_DECL_NSIINTERFACEREQUESTOR
        NS_DECL_NSITIMERCALLBACK
        NS_DECL_NSIDNSLISTENER

        nsHalfOpenSocket(nsConnectionEntry *ent,
                         nsHttpTransaction *trans);
        ~nsHalfOpenSocket();
        
        nsresult SetupStreams(nsISocketTransport **,
                              nsIAsyncInputStream **,
                              nsIAsyncOutputStream **,
                              bool isBackup);
        nsresult SetupPrimaryStreams();
        nsresult SetupBackupStreams();
        void     SetupBackupTimer();
        void     CancelBackupTimer();
        void     Abandon();
        
        nsHttpTransaction *Transaction() { return mTransaction; }

    private:
        nsConnectionEntry              *mEnt;
        nsRefPtr<nsHttpTransaction>    mTransaction;
        nsCOMPtr<nsISocketTransport>   mSocketTransport;
        nsCOMPtr<nsIAsyncOutputStream> mStreamOut;
        nsCOMPtr<nsIAsyncInputStream>  mStreamIn;

        
        nsCOMPtr<nsITimer>             mSynTimer;
        nsCOMPtr<nsISocketTransport>   mBackupTransport;
        nsCOMPtr<nsIAsyncOutputStream> mBackupStreamOut;
        nsCOMPtr<nsIAsyncInputStream>  mBackupStreamIn;
    };
    friend class nsHalfOpenSocket;

    
    
    

    PRInt32                      mRef;
    mozilla::ReentrantMonitor    mReentrantMonitor;
    nsCOMPtr<nsIEventTarget>     mSocketThreadTarget;

    
    PRUint16 mMaxConns;
    PRUint16 mMaxConnsPerHost;
    PRUint16 mMaxConnsPerProxy;
    PRUint16 mMaxPersistConnsPerHost;
    PRUint16 mMaxPersistConnsPerProxy;
    PRUint16 mMaxRequestDelay; 
    PRUint16 mMaxPipelinedRequests;

    bool mIsShuttingDown;

    
    
    

    static PLDHashOperator ProcessOneTransactionCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);

    static PLDHashOperator PruneDeadConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator ShutdownPassCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator PurgeExcessIdleConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator ClosePersistentConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    bool     ProcessPendingQForEntry(nsConnectionEntry *);
    bool     AtActiveConnectionLimit(nsConnectionEntry *, PRUint8 caps);
    void     GetConnection(nsConnectionEntry *, nsHttpTransaction *,
                           bool, nsHttpConnection **);
    nsresult DispatchTransaction(nsConnectionEntry *, nsHttpTransaction *,
                                 PRUint8 caps, nsHttpConnection *);
    bool     BuildPipeline(nsConnectionEntry *, nsAHttpTransaction *, nsHttpPipeline **);
    nsresult ProcessNewTransaction(nsHttpTransaction *);
    nsresult EnsureSocketThreadTargetIfOnline();
    void     ClosePersistentConnections(nsConnectionEntry *ent);
    nsresult CreateTransport(nsConnectionEntry *, nsHttpTransaction *);
    void     AddActiveConn(nsHttpConnection *, nsConnectionEntry *);
    void     StartedConnect();
    void     RecvdConnect();

    
    nsConnectionEntry *GetSpdyPreferred(nsACString &aDottedDecimal);
    void               SetSpdyPreferred(nsACString &aDottedDecimal,
                                        nsConnectionEntry *ent);
    void               RemoveSpdyPreferred(nsACString &aDottedDecimal);
    nsHttpConnection  *GetSpdyPreferredConn(nsConnectionEntry *ent);
    nsDataHashtable<nsCStringHashKey, nsConnectionEntry *>   mSpdyPreferredHash;

    void               ProcessSpdyPendingQ(nsConnectionEntry *ent);
    void               ProcessSpdyPendingQ();
    static PLDHashOperator ProcessSpdyPendingQCB(
        const nsACString &key, nsAutoPtr<nsConnectionEntry> &ent,
        void *closure);

    
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
    void OnMsgClosePersistentConnections (PRInt32, void *);

    
    
    PRUint16 mNumActiveConns;
    
    
    PRUint16 mNumIdleConns;

    
    PRUint64 mTimeOfNextWakeUp;
    
    nsCOMPtr<nsITimer> mTimer;

    
    
    
    
    
    
    nsClassHashtable<nsCStringHashKey, nsConnectionEntry> mCT;
};

#endif 
