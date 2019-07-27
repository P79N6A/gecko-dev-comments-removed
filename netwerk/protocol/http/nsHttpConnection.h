




#ifndef nsHttpConnection_h__
#define nsHttpConnection_h__

#include "nsHttpConnectionInfo.h"
#include "nsAHttpTransaction.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsProxyRelease.h"
#include "prinrval.h"
#include "TunnelUtils.h"

#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsITimer.h"

class nsISocketTransport;
class nsISSLSocketControl;

namespace mozilla {
namespace net {

class nsHttpHandler;
class ASpdySession;








class nsHttpConnection MOZ_FINAL : public nsAHttpSegmentReader
                                 , public nsAHttpSegmentWriter
                                 , public nsIInputStreamCallback
                                 , public nsIOutputStreamCallback
                                 , public nsITransportEventSink
                                 , public nsIInterfaceRequestor
                                 , public NudgeTunnelCallback
{
    virtual ~nsHttpConnection();

public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSAHTTPSEGMENTREADER
    NS_DECL_NSAHTTPSEGMENTWRITER
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSIOUTPUTSTREAMCALLBACK
    NS_DECL_NSITRANSPORTEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NUDGETUNNELCALLBACK

    nsHttpConnection();

    
    
    
    
    
    nsresult Init(nsHttpConnectionInfo *info, uint16_t maxHangTime,
                  nsISocketTransport *, nsIAsyncInputStream *,
                  nsIAsyncOutputStream *, bool connectedTransport,
                  nsIInterfaceRequestor *, PRIntervalTime);

    
    
    
    nsresult Activate(nsAHttpTransaction *, uint32_t caps, int32_t pri);

    
    void Close(nsresult reason);

    
    

    bool SupportsPipelining();
    bool IsKeepAlive()
    {
        return mUsingSpdyVersion || (mKeepAliveMask && mKeepAlive);
    }
    bool CanReuse();   
    bool CanDirectlyActivate();

    
    uint32_t TimeToLive();

    void DontReuse();

    bool IsProxyConnectInProgress()
    {
        return mProxyConnectInProgress;
    }

    bool LastTransactionExpectedNoContent()
    {
        return mLastTransactionExpectedNoContent;
    }

    void SetLastTransactionExpectedNoContent(bool val)
    {
        mLastTransactionExpectedNoContent = val;
    }

    bool NeedSpdyTunnel()
    {
        return mConnInfo->UsingHttpsProxy() && !mTLSFilter && mConnInfo->UsingConnect();
    }

    
    
    void ForcePlainText()
    {
        mForcePlainText = true;
    }

    nsISocketTransport   *Transport()      { return mSocketTransport; }
    nsAHttpTransaction   *Transaction()    { return mTransaction; }
    nsHttpConnectionInfo *ConnectionInfo() { return mConnInfo; }

    
    nsresult OnHeadersAvailable(nsAHttpTransaction *, nsHttpRequestHead *, nsHttpResponseHead *, bool *reset);
    void     CloseTransaction(nsAHttpTransaction *, nsresult reason);
    void     GetConnectionInfo(nsHttpConnectionInfo **ci) { NS_IF_ADDREF(*ci = mConnInfo); }
    nsresult TakeTransport(nsISocketTransport **,
                           nsIAsyncInputStream **,
                           nsIAsyncOutputStream **);
    void     GetSecurityInfo(nsISupports **);
    bool     IsPersistent() { return IsKeepAlive() && !mDontReuse; }
    bool     IsReused();
    void     SetIsReusedAfter(uint32_t afterMilliseconds);
    nsresult PushBack(const char *data, uint32_t length);
    nsresult ResumeSend();
    nsresult ResumeRecv();
    int64_t  MaxBytesRead() {return mMaxBytesRead;}
    uint8_t GetLastHttpResponseVersion() { return mLastHttpResponseVersion; }

    friend class nsHttpConnectionForceIO;
    nsresult ForceSend();
    nsresult ForceRecv();

    static NS_METHOD ReadFromStream(nsIInputStream *, void *, const char *,
                                    uint32_t, uint32_t, uint32_t *);

    
    
    
    
    void BeginIdleMonitoring();
    void EndIdleMonitoring();

    bool UsingSpdy() { return !!mUsingSpdyVersion; }
    uint8_t GetSpdyVersion() { return mUsingSpdyVersion; }
    bool EverUsedSpdy() { return mEverUsedSpdy; }
    PRIntervalTime Rtt() { return mRtt; }

    
    
    bool ReportedNPN() { return mReportedSpdy; }

    
    
    
    
    uint32_t  ReadTimeoutTick(PRIntervalTime now);

    
    
    
    static void UpdateTCPKeepalive(nsITimer *aTimer, void *aClosure);

    nsAHttpTransaction::Classifier Classification() { return mClassification; }
    void Classify(nsAHttpTransaction::Classifier newclass)
    {
        mClassification = newclass;
    }

    
    void  ReadTimeoutTick();

    int64_t BytesWritten() { return mTotalBytesWritten; } 
    int64_t ContentBytesWritten() { return mContentBytesWritten; }

    void    SetSecurityCallbacks(nsIInterfaceRequestor* aCallbacks);
    void    PrintDiagnostics(nsCString &log);

    void    SetTransactionCaps(uint32_t aCaps) { mTransactionCaps = aCaps; }

    
    
    bool    IsExperienced() { return mExperienced; }

    static nsresult MakeConnectString(nsAHttpTransaction *trans,
                                      nsHttpRequestHead *request,
                                      nsACString &result);
    void    SetupSecondaryTLS();
    void    SetInSpdyTunnel(bool arg);

private:
    
    enum TCPKeepaliveConfig {
      kTCPKeepaliveDisabled = 0,
      kTCPKeepaliveShortLivedConfig,
      kTCPKeepaliveLongLivedConfig
    };

    
    nsresult InitSSLParams(bool connectingToProxy, bool ProxyStartSSL);
    nsresult SetupNPNList(nsISSLSocketControl *ssl, uint32_t caps);

    nsresult OnTransactionDone(nsresult reason);
    nsresult OnSocketWritable();
    nsresult OnSocketReadable();

    nsresult SetupProxyConnect();

    PRIntervalTime IdleTime();
    bool     IsAlive();
    bool     SupportsPipelining(nsHttpResponseHead *);

    
    
    bool     EnsureNPNComplete();
    void     SetupSSL();

    
    void     StartSpdy(uint8_t versionLevel);

    
    nsresult AddTransaction(nsAHttpTransaction *, int32_t);

    
    
    nsresult StartShortLivedTCPKeepalives();
    nsresult StartLongLivedTCPKeepalives();
    nsresult DisableTCPKeepalives();

private:
    nsCOMPtr<nsISocketTransport>    mSocketTransport;
    nsCOMPtr<nsIAsyncInputStream>   mSocketIn;
    nsCOMPtr<nsIAsyncOutputStream>  mSocketOut;

    nsresult                        mSocketInCondition;
    nsresult                        mSocketOutCondition;

    nsCOMPtr<nsIInputStream>        mProxyConnectStream;
    nsCOMPtr<nsIInputStream>        mRequestStream;

    
    
    nsRefPtr<nsAHttpTransaction>    mTransaction;
    nsRefPtr<TLSFilterTransaction>  mTLSFilter;

    nsRefPtr<nsHttpHandler>         mHttpHandler; 

    Mutex                           mCallbacksLock;
    nsMainThreadPtrHandle<nsIInterfaceRequestor> mCallbacks;

    nsRefPtr<nsHttpConnectionInfo> mConnInfo;

    PRIntervalTime                  mLastReadTime;
    PRIntervalTime                  mLastWriteTime;
    PRIntervalTime                  mMaxHangTime;    
    PRIntervalTime                  mIdleTimeout;    
    PRIntervalTime                  mConsiderReusedAfterInterval;
    PRIntervalTime                  mConsiderReusedAfterEpoch;
    int64_t                         mCurrentBytesRead;   
    int64_t                         mMaxBytesRead;       
    int64_t                         mTotalBytesRead;     
    int64_t                         mTotalBytesWritten;  
    int64_t                         mContentBytesWritten;  

    nsRefPtr<nsIAsyncInputStream>   mInputOverflow;

    PRIntervalTime                  mRtt;

    bool                            mConnectedTransport;
    bool                            mKeepAlive;
    bool                            mKeepAliveMask;
    bool                            mDontReuse;
    bool                            mSupportsPipelining;
    bool                            mIsReused;
    bool                            mCompletedProxyConnect;
    bool                            mLastTransactionExpectedNoContent;
    bool                            mIdleMonitoring;
    bool                            mProxyConnectInProgress;
    bool                            mExperienced;
    bool                            mInSpdyTunnel;
    bool                            mForcePlainText;

    
    
    uint32_t                        mHttp1xTransactionCount;

    
    
    
    uint32_t                        mRemainingConnectionUses;

    nsAHttpTransaction::Classifier  mClassification;

    
    bool                            mNPNComplete;
    bool                            mSetupSSLCalled;

    
    uint8_t                         mUsingSpdyVersion;

    nsRefPtr<ASpdySession>          mSpdySession;
    int32_t                         mPriority;
    bool                            mReportedSpdy;

    
    bool                            mEverUsedSpdy;

    
    uint8_t                         mLastHttpResponseVersion;

    
    uint32_t                        mTransactionCaps;

    bool                            mResponseTimeoutEnabled;

    
    uint32_t                        mTCPKeepaliveConfig;
    nsCOMPtr<nsITimer>              mTCPKeepaliveTransitionTimer;
};

}} 

#endif
