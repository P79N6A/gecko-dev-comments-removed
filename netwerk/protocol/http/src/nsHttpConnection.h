





































#ifndef nsHttpConnection_h__
#define nsHttpConnection_h__

#include "nsHttp.h"
#include "nsHttpConnectionInfo.h"
#include "nsAHttpConnection.h"
#include "nsAHttpTransaction.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "prlock.h"

#include "nsIStreamListener.h"
#include "nsISocketTransport.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIInterfaceRequestor.h"








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

    
    
    
    
    
    nsresult Init(nsHttpConnectionInfo *info, PRUint16 maxHangTime);

    
    
    nsresult Activate(nsAHttpTransaction *, PRUint8 caps);

    
    void Close(nsresult reason);

    
    

    PRBool   SupportsPipelining() { return mSupportsPipelining; }
    PRBool   IsKeepAlive() { return mKeepAliveMask && mKeepAlive; }
    PRBool   CanReuse();   
    void     DontReuse()   { mKeepAliveMask = PR_FALSE;
                             mKeepAlive = PR_FALSE;
                             mIdleTimeout = 0; }
    void     DropTransport() { DontReuse(); mSocketTransport = 0; }

    nsAHttpTransaction   *Transaction()    { return mTransaction; }
    nsHttpConnectionInfo *ConnectionInfo() { return mConnInfo; }

    
    nsresult OnHeadersAvailable(nsAHttpTransaction *, nsHttpRequestHead *, nsHttpResponseHead *, PRBool *reset);
    void     CloseTransaction(nsAHttpTransaction *, nsresult reason);
    void     GetConnectionInfo(nsHttpConnectionInfo **ci) { NS_IF_ADDREF(*ci = mConnInfo); }
    void     GetSecurityInfo(nsISupports **);
    PRBool   IsPersistent() { return IsKeepAlive(); }
    PRBool   IsReused() { return mIsReused; }
    nsresult PushBack(const char *data, PRUint32 length) { NS_NOTREACHED("PushBack"); return NS_ERROR_UNEXPECTED; }
    nsresult ResumeSend();
    nsresult ResumeRecv();

    static NS_METHOD ReadFromStream(nsIInputStream *, void *, const char *,
                                    PRUint32, PRUint32, PRUint32 *);

private:
    
    nsresult ProxyStartSSL();

    nsresult CreateTransport();
    nsresult OnTransactionDone(nsresult reason);
    nsresult OnSocketWritable();
    nsresult OnSocketReadable();

    nsresult SetupSSLProxyConnect();

    PRBool   IsAlive();
    PRBool   SupportsPipelining(nsHttpResponseHead *);
    
private:
    nsCOMPtr<nsISocketTransport>    mSocketTransport;
    nsCOMPtr<nsIAsyncInputStream>   mSocketIn;
    nsCOMPtr<nsIAsyncOutputStream>  mSocketOut;

    nsresult                        mSocketInCondition;
    nsresult                        mSocketOutCondition;

    nsCOMPtr<nsIInputStream>        mSSLProxyConnectStream;
    nsCOMPtr<nsIInputStream>        mRequestStream;

    nsAHttpTransaction             *mTransaction; 
    nsHttpConnectionInfo           *mConnInfo;    

    PRLock                         *mLock;

    PRUint32                        mLastReadTime;
    PRUint16                        mMaxHangTime;    
    PRUint16                        mIdleTimeout;    

    PRPackedBool                    mKeepAlive;
    PRPackedBool                    mKeepAliveMask;
    PRPackedBool                    mSupportsPipelining;
    PRPackedBool                    mIsReused;
    PRPackedBool                    mCompletedSSLConnect;
};

#endif 
