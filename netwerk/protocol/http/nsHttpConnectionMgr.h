




#ifndef nsHttpConnectionMgr_h__
#define nsHttpConnectionMgr_h__

#include "nsHttpConnectionInfo.h"
#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "NullHttpTransaction.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsAutoPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsISocketTransportService.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"
#include "mozilla/net/DashboardTypes.h"

#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsIX509Cert3.h"

class nsHttpPipeline;

class nsIHttpUpgradeListener;



class nsHttpConnectionMgr : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    
    enum nsParamName {
        MAX_CONNECTIONS,
        MAX_PERSISTENT_CONNECTIONS_PER_HOST,
        MAX_PERSISTENT_CONNECTIONS_PER_PROXY,
        MAX_REQUEST_DELAY,
        MAX_PIPELINED_REQUESTS,
        MAX_OPTIMISTIC_PIPELINED_REQUESTS
    };

    
    
    

    nsHttpConnectionMgr();

    nsresult Init(uint16_t maxConnections,
                  uint16_t maxPersistentConnectionsPerHost,
                  uint16_t maxPersistentConnectionsPerProxy,
                  uint16_t maxRequestDelay,
                  uint16_t maxPipelinedRequests,
                  uint16_t maxOptimisticPipelinedRequests);
    nsresult Shutdown();

    
    
    

    
    
    void PruneDeadConnectionsAfter(uint32_t time);

    
    
    void ConditionallyStopPruneDeadConnectionsTimer();

    
    
    void ConditionallyStopTimeoutTick();

    
    nsresult AddTransaction(nsHttpTransaction *, int32_t priority);

    
    
    nsresult RescheduleTransaction(nsHttpTransaction *, int32_t priority);

    
    nsresult CancelTransaction(nsHttpTransaction *, nsresult reason);

    
    
    nsresult PruneDeadConnections();

    
    
    nsresult ClosePersistentConnections();

    
    
    nsresult GetSocketThreadTarget(nsIEventTarget **);

    
    
    
    
    
    
    nsresult SpeculativeConnect(nsHttpConnectionInfo *,
                                nsIInterfaceRequestor *);

    
    
    
    nsresult ReclaimConnection(nsHttpConnection *conn);

    
    
    
    
    nsresult CompleteUpgrade(nsAHttpConnection *aConn,
                             nsIHttpUpgradeListener *aUpgradeListener);

    
    
    nsresult UpdateParam(nsParamName name, uint16_t value);

    
    bool GetSpdyAlternateProtocol(nsACString &key);
    void ReportSpdyAlternateProtocol(nsHttpConnection *);
    void RemoveSpdyAlternateProtocol(nsACString &key);

    

    const static uint32_t kPipelineInfoTypeMask = 0xffff0000;
    const static uint32_t kPipelineInfoIDMask   = ~kPipelineInfoTypeMask;

    const static uint32_t kPipelineInfoTypeRed     = 0x00010000;
    const static uint32_t kPipelineInfoTypeBad     = 0x00020000;
    const static uint32_t kPipelineInfoTypeNeutral = 0x00040000;
    const static uint32_t kPipelineInfoTypeGood    = 0x00080000;

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
                                  uint32_t);

    void ReportFailedToProcess(nsIURI *uri);

    
    
    void PrintDiagnostics();

    
    
    

    
    
    nsresult ProcessPendingQ(nsHttpConnectionInfo *);
    bool     ProcessPendingQForEntry(nsHttpConnectionInfo *);

    
    nsresult ProcessPendingQ();

    
    
    
    nsresult CloseIdleConnection(nsHttpConnection *);

    
    
    
    void ReportSpdyConnection(nsHttpConnection *, bool usingSpdy);

    
    
    void ReportSpdyCWNDSetting(nsHttpConnectionInfo *host, uint32_t cwndValue);
    uint32_t GetSpdyCWNDSetting(nsHttpConnectionInfo *host);
    
    bool     SupportsPipelining(nsHttpConnectionInfo *);

    bool GetConnectionData(nsTArray<mozilla::net::HttpRetParams> *);

    void ResetIPFamillyPreference(nsHttpConnectionInfo *);

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

        
        
        uint32_t UnconnectedHalfOpens();

        
        void RemoveHalfOpen(nsHalfOpenSocket *);

        
        const static uint32_t kPipelineUnlimited  = 1024; 
        const static uint32_t kPipelineOpen       = 6;    
        const static uint32_t kPipelineRestricted = 2;    
        
        nsHttpConnectionMgr::PipeliningState PipelineState();
        void OnPipelineFeedbackInfo(
            nsHttpConnectionMgr::PipelineFeedbackInfoType info,
            nsHttpConnection *, uint32_t);
        bool SupportsPipelining();
        uint32_t MaxPipelineDepth(nsAHttpTransaction::Classifier classification);
        void CreditPenalty();

        nsHttpConnectionMgr::PipeliningState mPipelineState;

        void SetYellowConnection(nsHttpConnection *);
        void OnYellowComplete();
        uint32_t                  mYellowGoodEvents;
        uint32_t                  mYellowBadEvents;
        nsHttpConnection         *mYellowConnection;

        
        
        
        uint32_t                  mInitialGreenDepth;

        
        
        
        
        uint32_t                  mGreenDepth;

        
        
        
        
        
        
        int16_t                   mPipeliningPenalty;

        
        
        
        int16_t                   mPipeliningClassPenalty[nsAHttpTransaction::CLASS_MAX];

        
        mozilla::TimeStamp        mLastCreditTime;

        
        
        
        
        
        
        
        
        
        nsCString mCoalescingKey;

        
        
        uint32_t            mSpdyCWND;
        mozilla::TimeStamp  mSpdyCWNDTimeStamp;

        
        
        
        bool mUsingSpdy;

        
        
        
        
        bool mTestedSpdy;

        bool mSpdyPreferred;

        
        
        
        
        bool mPreferIPv4 : 1;
        
        
        bool mPreferIPv6 : 1;

        
        void RecordIPFamilyPreference(uint16_t family);
        
        void ResetIPFamilyPreference();
    };

    
    
    
    
    
    
    
    
    
    class nsConnectionHandle : public nsAHttpConnection
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSAHTTPCONNECTION(mConn)

        nsConnectionHandle(nsHttpConnection *conn) { NS_ADDREF(mConn = conn); }
        virtual ~nsConnectionHandle();

        nsHttpConnection *mConn;
    };

    
    

    class nsHalfOpenSocket MOZ_FINAL : public nsIOutputStreamCallback,
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
                         nsAHttpTransaction *trans,
                         uint32_t caps);
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
        double   Duration(mozilla::TimeStamp epoch);
        nsISocketTransport *SocketTransport() { return mSocketTransport; }
        nsISocketTransport *BackupTransport() { return mBackupTransport; }

        nsAHttpTransaction *Transaction() { return mTransaction; }

        bool IsSpeculative() { return mSpeculative; }
        void SetSpeculative(bool val) { mSpeculative = val; }

        bool HasConnected() { return mHasConnected; }

        void PrintDiagnostics(nsCString &log);
    private:
        nsConnectionEntry              *mEnt;
        nsRefPtr<nsAHttpTransaction>   mTransaction;
        nsCOMPtr<nsISocketTransport>   mSocketTransport;
        nsCOMPtr<nsIAsyncOutputStream> mStreamOut;
        nsCOMPtr<nsIAsyncInputStream>  mStreamIn;
        uint32_t                       mCaps;

        
        
        
        
        
        
        
        bool                           mSpeculative;

        mozilla::TimeStamp             mPrimarySynStarted;
        mozilla::TimeDuration          mPrimarySynRTT;
        mozilla::TimeStamp             mBackupSynStarted;

        
        nsCOMPtr<nsITimer>             mSynTimer;
        nsCOMPtr<nsISocketTransport>   mBackupTransport;
        nsCOMPtr<nsIAsyncOutputStream> mBackupStreamOut;
        nsCOMPtr<nsIAsyncInputStream>  mBackupStreamIn;

        bool                           mHasConnected;
    };
    friend class nsHalfOpenSocket;

    
    
    

    mozilla::ReentrantMonitor    mReentrantMonitor;
    nsCOMPtr<nsIEventTarget>     mSocketThreadTarget;

    
    uint16_t mMaxConns;
    uint16_t mMaxPersistConnsPerHost;
    uint16_t mMaxPersistConnsPerProxy;
    uint16_t mMaxRequestDelay; 
    uint16_t mMaxPipelinedRequests;
    uint16_t mMaxOptimisticPipelinedRequests;
    bool mIsShuttingDown;

    
    
    

    static PLDHashOperator ProcessOneTransactionCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator ProcessAllTransactionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);

    static PLDHashOperator PruneDeadConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator ShutdownPassCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator PurgeExcessIdleConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator PurgeExcessSpdyConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    static PLDHashOperator ClosePersistentConnectionsCB(const nsACString &, nsAutoPtr<nsConnectionEntry> &, void *);
    bool     ProcessPendingQForEntry(nsConnectionEntry *, bool considerAll);
    bool     IsUnderPressure(nsConnectionEntry *ent,
                             nsHttpTransaction::Classifier classification);
    bool     AtActiveConnectionLimit(nsConnectionEntry *, uint32_t caps);
    nsresult TryDispatchTransaction(nsConnectionEntry *ent,
                                    bool onlyReusedConnection,
                                    nsHttpTransaction *trans);
    nsresult DispatchTransaction(nsConnectionEntry *,
                                 nsHttpTransaction *,
                                 nsHttpConnection *);
    nsresult DispatchAbstractTransaction(nsConnectionEntry *,
                                         nsAHttpTransaction *,
                                         uint32_t,
                                         nsHttpConnection *,
                                         int32_t);
    nsresult BuildPipeline(nsConnectionEntry *,
                           nsAHttpTransaction *,
                           nsHttpPipeline **);
    bool     RestrictConnections(nsConnectionEntry *);
    nsresult ProcessNewTransaction(nsHttpTransaction *);
    nsresult EnsureSocketThreadTarget();
    void     ClosePersistentConnections(nsConnectionEntry *ent);
    void     ReportProxyTelemetry(nsConnectionEntry *ent);
    nsresult CreateTransport(nsConnectionEntry *, nsAHttpTransaction *,
                             uint32_t, bool);
    void     AddActiveConn(nsHttpConnection *, nsConnectionEntry *);
    void     StartedConnect();
    void     RecvdConnect();

    nsConnectionEntry *GetOrCreateConnectionEntry(nsHttpConnectionInfo *);

    nsresult MakeNewConnection(nsConnectionEntry *ent,
                               nsHttpTransaction *trans);
    bool     AddToShortestPipeline(nsConnectionEntry *ent,
                                   nsHttpTransaction *trans,
                                   nsHttpTransaction::Classifier classification,
                                   uint16_t depthLimit);

    
    nsConnectionEntry *GetSpdyPreferredEnt(nsConnectionEntry *aOriginalEntry);
    void               RemoveSpdyPreferredEnt(nsACString &aDottedDecimal);
    nsHttpConnection  *GetSpdyPreferredConn(nsConnectionEntry *ent);
    nsDataHashtable<nsCStringHashKey, nsConnectionEntry *>   mSpdyPreferredHash;
    nsConnectionEntry *LookupConnectionEntry(nsHttpConnectionInfo *ci,
                                             nsHttpConnection *conn,
                                             nsHttpTransaction *trans);

    void               ProcessSpdyPendingQ(nsConnectionEntry *ent);
    static PLDHashOperator ProcessSpdyPendingQCB(
        const nsACString &key, nsAutoPtr<nsConnectionEntry> &ent,
        void *closure);

    
    typedef void (nsHttpConnectionMgr:: *nsConnEventHandler)(int32_t, void *);

    
    
    
    
    
    class nsConnEvent;
    friend class nsConnEvent;
    class nsConnEvent : public nsRunnable
    {
    public:
        nsConnEvent(nsHttpConnectionMgr *mgr,
                    nsConnEventHandler handler,
                    int32_t iparam,
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
        int32_t              mIParam;
        void                *mVParam;
    };

    nsresult PostEvent(nsConnEventHandler  handler,
                       int32_t             iparam = 0,
                       void               *vparam = nullptr);

    
    void OnMsgShutdown             (int32_t, void *);
    void OnMsgShutdownConfirm      (int32_t, void *);
    void OnMsgNewTransaction       (int32_t, void *);
    void OnMsgReschedTransaction   (int32_t, void *);
    void OnMsgCancelTransaction    (int32_t, void *);
    void OnMsgProcessPendingQ      (int32_t, void *);
    void OnMsgPruneDeadConnections (int32_t, void *);
    void OnMsgSpeculativeConnect   (int32_t, void *);
    void OnMsgReclaimConnection    (int32_t, void *);
    void OnMsgCompleteUpgrade      (int32_t, void *);
    void OnMsgUpdateParam          (int32_t, void *);
    void OnMsgClosePersistentConnections (int32_t, void *);
    void OnMsgProcessFeedback      (int32_t, void *);
    void OnMsgProcessAllSpdyPendingQ (int32_t, void *);

    
    
    uint16_t mNumActiveConns;
    
    
    uint16_t mNumIdleConns;
    
    
    uint32_t mNumHalfOpenConns;

    
    uint64_t mTimeOfNextWakeUp;
    
    nsCOMPtr<nsITimer> mTimer;

    
    
    
    nsCOMPtr<nsITimer> mTimeoutTick;
    bool mTimeoutTickArmed;

    
    
    
    
    
    
    nsClassHashtable<nsCStringHashKey, nsConnectionEntry> mCT;

    
    
    nsTHashtable<nsCStringHashKey> mAlternateProtocolHash;
    static PLDHashOperator TrimAlternateProtocolHash(nsCStringHashKey *entry,
                                                     void *closure);

    static PLDHashOperator ReadConnectionEntry(const nsACString &key,
                                               nsAutoPtr<nsConnectionEntry> &ent,
                                               void *aArg);

    
    void ActivateTimeoutTick();
    void TimeoutTick();
    static PLDHashOperator TimeoutTickCB(const nsACString &key,
                                         nsAutoPtr<nsConnectionEntry> &ent,
                                         void *closure);

    
    void OnMsgPrintDiagnostics(int32_t, void *);
    static PLDHashOperator PrintDiagnosticsCB(const nsACString &key,
                                              nsAutoPtr<nsConnectionEntry> &ent,
                                              void *closure);
    nsCString mLogData;
};

#endif 
