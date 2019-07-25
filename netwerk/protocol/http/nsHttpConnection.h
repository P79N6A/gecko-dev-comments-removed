





































#ifndef nsHttpConnection_h__
#define nsHttpConnection_h__

#include "nsHttp.h"
#include "nsHttpConnectionInfo.h"
#include "nsAHttpConnection.h"
#include "nsAHttpTransaction.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "prinrval.h"
#include "SpdySession.h"

#include "nsIStreamListener.h"
#include "nsISocketTransport.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsIEventTarget.h"








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

    
    
    
    
    
    nsresult Init(nsHttpConnectionInfo *info, PRUint16 maxHangTime,
                  nsISocketTransport *, nsIAsyncInputStream *,
                  nsIAsyncOutputStream *, nsIInterfaceRequestor *,
                  nsIEventTarget *);

    
    
    
    nsresult Activate(nsAHttpTransaction *, PRUint8 caps, PRInt32 pri);

    
    void Close(nsresult reason);

    
    

    bool     SupportsPipelining() { return mSupportsPipelining; }
    bool     IsKeepAlive() { return mUsingSpdy ||
                                    (mKeepAliveMask && mKeepAlive); }
    bool     CanReuse();   
    bool     CanDirectlyActivate();

    
    PRUint32 TimeToLive();

    void     DontReuse();
    void     DropTransport() { DontReuse(); mSocketTransport = 0; }

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
    void     SetIsReusedAfter(PRUint32 afterMilliseconds);
    void     SetIdleTimeout(PRUint16 val) {mIdleTimeout = val;}
    nsresult PushBack(const char *data, PRUint32 length);
    nsresult ResumeSend(nsAHttpTransaction *caller);
    nsresult ResumeRecv(nsAHttpTransaction *caller);
    PRInt64  MaxBytesRead() {return mMaxBytesRead;}

    static NS_METHOD ReadFromStream(nsIInputStream *, void *, const char *,
                                    PRUint32, PRUint32, PRUint32 *);

    
    
    
    
    void BeginIdleMonitoring();
    void EndIdleMonitoring();

    bool UsingSpdy() { return mUsingSpdy; }

private:
    
    nsresult ProxyStartSSL();

    nsresult OnTransactionDone(nsresult reason);
    nsresult OnSocketWritable();
    nsresult OnSocketReadable();

    nsresult SetupProxyConnect();

    bool     IsAlive();
    bool     SupportsPipelining(nsHttpResponseHead *);
    
    
    
    bool     EnsureNPNComplete();
    void     SetupNPN(PRUint8 caps);

    
    
    void     HandleAlternateProtocol(nsHttpResponseHead *);

    
    nsresult AddTransaction(nsAHttpTransaction *, PRInt32);

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

    PRUint32                        mLastReadTime;
    PRUint16                        mMaxHangTime;    
    PRUint16                        mIdleTimeout;    
    PRIntervalTime                  mConsiderReusedAfterInterval;
    PRIntervalTime                  mConsiderReusedAfterEpoch;
    PRInt64                         mCurrentBytesRead;   
    PRInt64                         mMaxBytesRead;       

    nsRefPtr<nsIAsyncInputStream>   mInputOverflow;

    bool                            mKeepAlive;
    bool                            mKeepAliveMask;
    bool                            mSupportsPipelining;
    bool                            mIsReused;
    bool                            mCompletedProxyConnect;
    bool                            mLastTransactionExpectedNoContent;
    bool                            mIdleMonitoring;

    
    bool                            mNPNComplete;
    bool                            mSetupNPNCalled;
    bool                            mUsingSpdy;
    nsRefPtr<mozilla::net::SpdySession> mSpdySession;
    PRInt32                         mPriority;
    bool                            mReportedSpdy;
};

#endif 
