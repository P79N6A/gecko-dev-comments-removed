





































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
#include "mozilla/TimeStamp.h"

#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsIX509Cert3.h"

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
        MAX_PIPELINED_REQUESTS,
        MAX_OPTIMISTIC_PIPELINED_REQUESTS
    };

    
    
    

    nsHttpConnectionMgr();

    nsresult Init(PRUint16 maxConnections,
                  PRUint16 maxConnectionsPerHost,
                  PRUint16 maxConnectionsPerProxy,
                  PRUint16 maxPersistentConnectionsPerHost,
                  PRUint16 maxPersistentConnectionsPerProxy,
                  PRUint16 maxRequestDelay,
                  PRUint16 maxPipelinedRequests,
                  PRUint16 maxOptimisticPipelinedRequests);
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

    
    bool GetSpdyAlternateProtocol(nsACString &key);
    void ReportSpdyAlternateProtocol(nsHttpConnection *);
    void RemoveSpdyAlternateProtocol(nsACString &key);

    

    const static PRUint32 kPipelineInfoTypeMask = 0xffff0000;
    const static PRUint32 kPipelineInfoIDMask   = ~kPipelineInfoTypeMask;

    const static PRUint32 kPipelineInfoTypeRed     = 0x00010000;
    const static PRUint32 kPipelineInfoTypeBad     = 0x00020000;
    const static PRUint32 kPipelineInfoTypeNeutral = 0x00040000;
    const static PRUint32 kPipelineInfoTypeGood    = 0x00080000;

    enum PipelineFeedbackInfoType
    {
        
        RedVersionTooLow = kPipelineInfoTypeRed | kPipelineInfoTypeBad | 0x0001,

        
        
        RedBannedServer = kPipelineInfoTypeRed | kPipelineInfoTypeBad | 0x0002,
    
        
        
        
        RedCorruptedContent = kPipelineInfoTypeRed | kPipelineInfoTypeBad | 0x0004,

        
        
        
        RedCanceledPipeline = kPipelineInfoTypeRed | kPipelineInfoTypeBad | 0x0005,

        
        
        BadExplicitClose = kPipelineInfoTypeBad | 0x0003,

        
        
        BadSlowReadMinor = kPipelineInfoTypeBad | 0x0006,

        
        
        BadSlowReadMajor = kPipelineInfoTypeBad | 0x0007,

        
        
        BadInsufficientFraming = kPipelineInfoTypeBad | 0x0008,
        
        
        
        BadUnexpectedLarge = kPipelineInfoTypeBad | 0x000B,

        
        
        NeutralExpectedOK = kPipelineInfoTypeNeutral | 0x0009,

        
        GoodCompletedOK = kPipelineInfoTypeGood | 0x000A
    };
    
    
    
    void     PipelineFeedbackInfo(nsHttpConnectionInfo *,
                                  PipelineFeedbackInfoType info,
                                  nsHttpConnection *,
                                  PRUint32);

    void ReportFailedToProcess(nsIURI *uri);

    
    
    

    
    
    nsresult ProcessPendingQ(nsHttpConnectionInfo *);
    bool     ProcessPendingQForEntry(nsHttpConnectionInfo *);

    
    
    
    nsresult CloseIdleConnection(nsHttpConnection *);

    
    
    
    void ReportSpdyConnection(nsHttpConnection *, bool usingSpdy);

    
    bool     SupportsPipelining(nsHttpConnectionInfo *);

private:
    virtual ~nsHttpConnectionMgr();

    enum PipeliningState {
        
        
        PS_GREEN,

        
        
        
        PS_YELLOW,

        
        
        
        
        PS_RED
    };
    
    class nsHalfOpenSocket;

    
    
    
    
    
    
    class nsConnectionEntry
    {
    public:
        nsConnectionEntry(nsHttpConnectionInfo *ci);
        ~nsConnectionEntry();

        nsHttpConnectionInfo        *mConnInfo;
        nsTArray<nsHttpTransaction*> mPendingQ;    
        nsTArray<nsHttpConnection*>  mActiveConns; 
        nsTArray<nsHttpConnection*>  mIdleConns;   
        nsTArray<nsHalfOpenSocket*>  mHalfOpens;

        
        const static PRUint32 kPipelineUnlimited  = 1024; 
        const static PRUint32 kPipelineOpen       = 6;    
        const static PRUint32 kPipelineRestricted = 2;    
        
        nsHttpConnectionMgr::PipeliningState PipelineState();
        void OnPipelineFeedbackInfo(
            nsHttpConnectionMgr::PipelineFeedbackInfoType info,
            nsHttpConnection *, PRUint32);
        bool SupportsPipelining();
        PRUint32 MaxPipelineDepth(nsAHttpTransaction::Classifier classification);
        void CreditPenalty();

        nsHttpConnectionMgr::PipeliningState mPipelineState;

        void SetYellowConnection(nsHttpConnection *);
        void OnYellowComplete();
        PRUint32                  mYellowGoodEvents;
        PRUint32                  mYellowBadEvents;
        nsHttpConnection         *mYellowConnection;

        
        
        
        PRUint32                  mInitialGreenDepth;

        
        
        
        
        PRUint32                  mGreenDepth;

        
        
        
        
        
        
        PRInt16                   mPipeliningPenalty;

        
        
        
        PRInt16                   mPipeliningClassPenalty[nsAHttpTransaction::CLASS_MAX];

        
        mozilla::TimeStamp        mLastCreditTime;

        
        
        
        
        
        
        
        
        
        nsCString mCoalescingKey;

        
        
        
        bool mUsingSpdy;

        
        
        
        
        bool mTestedSpdy;

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
                             public nsITimerCallback
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOUTPUTSTREAMCALLBACK
        NS_DECL_NSITRANSPORTEVENTSINK
        NS_DECL_NSIINTERFACEREQUESTOR
        NS_DECL_NSITIMERCALLBACK

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

        mozilla::TimeStamp             mPrimarySynStarted;
        mozilla::TimeStamp             mBackupSynStarted;

        
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
    PRUint16 mMaxOptimisticPipelinedRequests;
    bool mIsShuttingDown;

    
    
    

    static PLDHashOperator ProcessOneTransactionCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);

    static PLDHashOperator PruneDeadConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator ShutdownPassCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator PurgeExcessIdleConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator ClosePersistentConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    bool     ProcessPendingQForEntry(nsConnectionEntry *);
    bool     IsUnderPressure(nsConnectionEntry *ent,
                             nsHttpTransaction::Classifier classification);
    bool     AtActiveConnectionLimit(nsConnectionEntry *, PRUint8 caps);
    nsresult TryDispatchTransaction(nsConnectionEntry *ent,
                                    bool onlyReusedConnection,
                                    nsHttpTransaction *trans);
    nsresult DispatchTransaction(nsConnectionEntry *,
                                 nsHttpTransaction *,
                                 nsHttpConnection *);
    nsresult BuildPipeline(nsConnectionEntry *,
                           nsAHttpTransaction *,
                           nsHttpPipeline **);
    nsresult ProcessNewTransaction(nsHttpTransaction *);
    nsresult EnsureSocketThreadTargetIfOnline();
    void     ClosePersistentConnections(nsConnectionEntry *ent);
    nsresult CreateTransport(nsConnectionEntry *, nsHttpTransaction *);
    void     AddActiveConn(nsHttpConnection *, nsConnectionEntry *);
    void     StartedConnect();
    void     RecvdConnect();

    bool     MakeNewConnection(nsConnectionEntry *ent,
                               nsHttpTransaction *trans);
    bool     AddToShortestPipeline(nsConnectionEntry *ent,
                                   nsHttpTransaction *trans,
                                   nsHttpTransaction::Classifier classification,
                                   PRUint16 depthLimit);

    
    nsConnectionEntry *GetSpdyPreferredEnt(nsConnectionEntry *aOriginalEntry);
    void               RemoveSpdyPreferredEnt(nsACString &aDottedDecimal);
    nsHttpConnection  *GetSpdyPreferredConn(nsConnectionEntry *ent);
    nsDataHashtable<nsCStringHashKey, nsConnectionEntry *>   mSpdyPreferredHash;
    nsConnectionEntry *LookupConnectionEntry(nsHttpConnectionInfo *ci,
                                             nsHttpConnection *conn,
                                             nsHttpTransaction *trans);

    void               ProcessSpdyPendingQ(nsConnectionEntry *ent);
    void               ProcessAllSpdyPendingQ();
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
    void OnMsgProcessFeedback      (PRInt32, void *);

    
    
    PRUint16 mNumActiveConns;
    
    
    PRUint16 mNumIdleConns;

    
    PRUint64 mTimeOfNextWakeUp;
    
    nsCOMPtr<nsITimer> mTimer;

    
    
    
    nsCOMPtr<nsITimer> mReadTimeoutTick;
    bool mReadTimeoutTickArmed;

    
    
    
    
    
    
    nsClassHashtable<nsCStringHashKey, nsConnectionEntry> mCT;

    
    
    nsTHashtable<nsCStringHashKey> mAlternateProtocolHash;
    static PLDHashOperator TrimAlternateProtocolHash(nsCStringHashKey *entry,
                                                     void *closure);
    
    void ActivateTimeoutTick();
    void ReadTimeoutTick();
    static PLDHashOperator ReadTimeoutTickCB(const nsACString &key,
                                             nsAutoPtr<nsConnectionEntry> &ent,
                                             void *closure);
};

#endif 
