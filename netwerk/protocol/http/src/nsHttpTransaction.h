





































#ifndef nsHttpTransaction_h__
#define nsHttpTransaction_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "nsCOMPtr.h"
#include "nsInt64.h"

#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsISocketTransportService.h"
#include "nsITransport.h"
#include "nsIEventTarget.h"



class nsHttpTransaction;
class nsHttpRequestHead;
class nsHttpResponseHead;
class nsHttpChunkedDecoder;
class nsIHttpActivityObserver;






class nsHttpTransaction : public nsAHttpTransaction
                        , public nsIInputStreamCallback
                        , public nsIOutputStreamCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSAHTTPTRANSACTION
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSIOUTPUTSTREAMCALLBACK

    nsHttpTransaction();
    virtual ~nsHttpTransaction();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsresult Init(PRUint8                caps,
                  nsHttpConnectionInfo  *connInfo,
                  nsHttpRequestHead     *reqHeaders,
                  nsIInputStream        *reqBody,
                  PRBool                 reqBodyIncludesHeaders,
                  nsIEventTarget        *consumerTarget,
                  nsIInterfaceRequestor *callbacks,
                  nsITransportEventSink *eventsink,
                  nsIAsyncInputStream  **responseBody);

    
    PRUint8                Caps()           { return mCaps; }
    nsHttpConnectionInfo  *ConnectionInfo() { return mConnInfo; }
    nsHttpRequestHead     *RequestHead()    { return mRequestHead; }
    nsHttpResponseHead    *ResponseHead()   { return mHaveAllHeaders ? mResponseHead : nsnull; }
    nsISupports           *SecurityInfo()   { return mSecurityInfo; }

    nsIInterfaceRequestor *Callbacks()      { return mCallbacks; } 
    nsIEventTarget        *ConsumerTarget() { return mConsumerTarget; }
    nsAHttpConnection     *Connection()     { return mConnection; }

    
    
    nsHttpResponseHead *TakeResponseHead();

    
    PRBool ResponseIsComplete() { return mResponseIsComplete; }

    void   SetSSLConnectFailed() { mSSLConnectFailed = PR_TRUE; }
    PRBool    SSLConnectFailed() { return mSSLConnectFailed; }

    
    void    SetPriority(PRInt32 priority) { mPriority = priority; }
    PRInt32    Priority()                 { return mPriority; }

private:
    nsresult Restart();
    void     ParseLine(char *line);
    nsresult ParseLineSegment(char *seg, PRUint32 len);
    nsresult ParseHead(char *, PRUint32 count, PRUint32 *countRead);
    nsresult HandleContentStart();
    nsresult HandleContent(char *, PRUint32 count, PRUint32 *contentRead, PRUint32 *contentRemaining);
    nsresult ProcessData(char *, PRUint32, PRUint32 *);
    void     DeleteSelfOnConsumerThread();

    static NS_METHOD ReadRequestSegment(nsIInputStream *, void *, const char *,
                                        PRUint32, PRUint32, PRUint32 *);
    static NS_METHOD WritePipeSegment(nsIOutputStream *, void *, char *,
                                      PRUint32, PRUint32, PRUint32 *);

private:
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsITransportEventSink> mTransportSink;
    nsCOMPtr<nsIEventTarget>        mConsumerTarget;
    nsCOMPtr<nsISupports>           mSecurityInfo;
    nsCOMPtr<nsIAsyncInputStream>   mPipeIn;
    nsCOMPtr<nsIAsyncOutputStream>  mPipeOut;

    nsCOMPtr<nsISupports>             mChannel;
    nsCOMPtr<nsIHttpActivityObserver> mActivityDistributor;

    nsCString                       mReqHeaderBuf;    
    nsCOMPtr<nsIInputStream>        mRequestStream;
    PRUint32                        mRequestSize;

    nsAHttpConnection              *mConnection;      
    nsHttpConnectionInfo           *mConnInfo;        
    nsHttpRequestHead              *mRequestHead;     
    nsHttpResponseHead             *mResponseHead;    

    nsAHttpSegmentReader           *mReader;
    nsAHttpSegmentWriter           *mWriter;

    nsCString                       mLineBuf;         

    nsInt64                         mContentLength;   
    nsInt64                         mContentRead;     

    nsHttpChunkedDecoder           *mChunkedDecoder;

    nsresult                        mStatus;

    PRInt16                         mPriority;

    PRUint16                        mRestartCount;        
    PRUint8                         mCaps;

    
    PRUint32                        mClosed             : 1;
    PRUint32                        mConnected          : 1;
    PRUint32                        mHaveStatusLine     : 1;
    PRUint32                        mHaveAllHeaders     : 1;
    PRUint32                        mTransactionDone    : 1;
    PRUint32                        mResponseIsComplete : 1;
    PRUint32                        mDidContentStart    : 1;
    PRUint32                        mNoContent          : 1; 
    PRUint32                        mSentData           : 1;
    PRUint32                        mReceivedData       : 1;
    PRUint32                        mStatusEventPending : 1;
    PRUint32                        mHasRequestBody     : 1;
    PRUint32                        mSSLConnectFailed   : 1;

    
    
    
};

#endif 
