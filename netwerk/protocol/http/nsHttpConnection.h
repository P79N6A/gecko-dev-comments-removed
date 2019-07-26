




#ifndef nsHttpConnection_h__
#define nsHttpConnection_h__

#include "nsHttp.h"
#include "nsHttpConnectionInfo.h"
#include "nsAHttpTransaction.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "prinrval.h"
#include "ASpdySession.h"
#include "mozilla/TimeStamp.h"

#include "nsIStreamListener.h"
#include "nsISocketTransport.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsIEventTarget.h"

class nsHttpRequestHead;
class nsHttpResponseHead;








class nsHttpConnection : public nsAHttpSegmentReader
                       , public nsAHttpSegmentWriter
                       , public nsIInputStreamCallback
                       , public nsIOutputStreamCallback
                       , public nsITransportEventSink
                       , public nsIInterfaceRequestor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSAHTTPSEGMENTREADER
    NS_DECL_NSAHTTPSEGMENTWRITER
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSIOUTPUTSTREAMCALLBACK
    NS_DECL_NSITRANSPORTEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR

    nsHttpConnection();
    virtual ~nsHttpConnection();

    
    
    
    
    
    nsresult Init(nsHttpConnectionInfo *info, uint16_t maxHangTime,
                  nsISocketTransport *, nsIAsyncInputStream *,
                  nsIAsyncOutputStream *, nsIInterfaceRequestor *,
                  nsIEventTarget *, PRIntervalTime);

    
    
    
    nsresult Activate(nsAHttpTransaction *, uint8_t caps, int32_t pri);

    
    void Close(nsresult reason);

    
    

    bool     SupportsPipelining();
    bool     IsKeepAlive() { return mUsingSpdyVersion ||
                                    (mKeepAliveMask && mKeepAlive); }
    bool     CanReuse();   
    bool     CanDirectlyActivate();

    
    uint32_t TimeToLive();

    void     DontReuse();
    void     DropTransport() { DontReuse(); mSocketTransport = 0; }

    bool     IsProxyConnectInProgress()
    {
        return mProxyConnectInProgress;
    }

    bool     LastTransactionExpectedNoContent()
    {
        return mLastTransactionExpectedNoContent;
    }

    void     SetLastTransactionExpectedNoContent(bool val)
    {
        mLastTransactionExpectedNoContent = val;
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
    bool     IsPersistent() { return IsKeepAlive(); }
    bool     IsReused();
    void     SetIsReusedAfter(uint32_t afterMilliseconds);
    void     SetIdleTimeout(PRIntervalTime val) {mIdleTimeout = val;}
    nsresult PushBack(const char *data, uint32_t length);
    nsresult ResumeSend();
    nsresult ResumeRecv();
    int64_t  MaxBytesRead() {return mMaxBytesRead;}

    static NS_METHOD ReadFromStream(nsIInputStream *, void *, const char *,
                                    uint32_t, uint32_t, uint32_t *);

    
    
    
    
    void BeginIdleMonitoring();
    void EndIdleMonitoring();

    bool UsingSpdy() { return !!mUsingSpdyVersion; }
    bool EverUsedSpdy() { return mEverUsedSpdy; }

    
    
    bool ReportedNPN() { return mReportedSpdy; }

    
    void  ReadTimeoutTick(PRIntervalTime now);

    nsAHttpTransaction::Classifier Classification() { return mClassification; }
    void Classify(nsAHttpTransaction::Classifier newclass)
    {
        mClassification = newclass;
    }

    
    void  ReadTimeoutTick();

    int64_t BytesWritten() { return mTotalBytesWritten; }

    void    PrintDiagnostics(nsCString &log);

private:
    
    nsresult ProxyStartSSL();

    nsresult OnTransactionDone(nsresult reason);
    nsresult OnSocketWritable();
    nsresult OnSocketReadable();

    nsresult SetupProxyConnect();

    PRIntervalTime IdleTime();
    bool     IsAlive();
    bool     SupportsPipelining(nsHttpResponseHead *);
    
    
    
    bool     EnsureNPNComplete();
    void     SetupNPN(uint8_t caps);

    
    
    void     HandleAlternateProtocol(nsHttpResponseHead *);

    
    void     StartSpdy(uint8_t versionLevel);

    
    nsresult AddTransaction(nsAHttpTransaction *, int32_t);

private:
    nsCOMPtr<nsISocketTransport>    mSocketTransport;
    nsCOMPtr<nsIAsyncInputStream>   mSocketIn;
    nsCOMPtr<nsIAsyncOutputStream>  mSocketOut;

    nsresult                        mSocketInCondition;
    nsresult                        mSocketOutCondition;

    nsCOMPtr<nsIInputStream>        mProxyConnectStream;
    nsCOMPtr<nsIInputStream>        mRequestStream;

    
    
    nsRefPtr<nsAHttpTransaction>    mTransaction;

    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsIEventTarget>        mCallbackTarget;

    nsRefPtr<nsHttpConnectionInfo> mConnInfo;

    PRIntervalTime                  mLastReadTime;
    PRIntervalTime                  mMaxHangTime;    
    PRIntervalTime                  mIdleTimeout;    
    PRIntervalTime                  mConsiderReusedAfterInterval;
    PRIntervalTime                  mConsiderReusedAfterEpoch;
    int64_t                         mCurrentBytesRead;   
    int64_t                         mMaxBytesRead;       
    int64_t                         mTotalBytesRead;     
    int64_t                         mTotalBytesWritten;  

    nsRefPtr<nsIAsyncInputStream>   mInputOverflow;

    PRIntervalTime                  mRtt;

    bool                            mKeepAlive;
    bool                            mKeepAliveMask;
    bool                            mDontReuse;
    bool                            mSupportsPipelining;
    bool                            mIsReused;
    bool                            mCompletedProxyConnect;
    bool                            mLastTransactionExpectedNoContent;
    bool                            mIdleMonitoring;
    bool                            mProxyConnectInProgress;

    
    
    uint32_t                        mHttp1xTransactionCount;

    
    
    
    uint32_t                        mRemainingConnectionUses;

    nsAHttpTransaction::Classifier  mClassification;

    
    bool                            mNPNComplete;
    bool                            mSetupNPNCalled;

    
    uint8_t                         mUsingSpdyVersion;

    nsRefPtr<mozilla::net::ASpdySession> mSpdySession;
    int32_t                         mPriority;
    bool                            mReportedSpdy;

    
    bool                            mEverUsedSpdy;
};

#endif 
