




#ifndef nsHttpTransaction_h__
#define nsHttpTransaction_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "nsCOMPtr.h"

#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsISocketTransportService.h"
#include "nsITransport.h"
#include "nsIEventTarget.h"
#include "TimingStruct.h"



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
                  bool                   reqBodyIncludesHeaders,
                  nsIEventTarget        *consumerTarget,
                  nsIInterfaceRequestor *callbacks,
                  nsITransportEventSink *eventsink,
                  nsIAsyncInputStream  **responseBody);

    
    nsHttpConnectionInfo  *ConnectionInfo() { return mConnInfo; }
    nsHttpResponseHead    *ResponseHead()   { return mHaveAllHeaders ? mResponseHead : nullptr; }
    nsISupports           *SecurityInfo()   { return mSecurityInfo; }

    nsIInterfaceRequestor *Callbacks()      { return mCallbacks; } 
    nsIEventTarget        *ConsumerTarget() { return mConsumerTarget; }

    
    
    nsHttpResponseHead *TakeResponseHead();

    
    bool ResponseIsComplete() { return mResponseIsComplete; }

    bool      SSLConnectFailed() { return mSSLConnectFailed; }

    
    void    SetPriority(PRInt32 priority) { mPriority = priority; }
    PRInt32    Priority()                 { return mPriority; }

    const TimingStruct& Timings() const { return mTimings; }
    enum Classifier Classification() { return mClassification; }

    void PrintDiagnostics(nsCString &log);

private:
    nsresult Restart();
    nsresult RestartInProgress();
    char    *LocateHttpStart(char *buf, PRUint32 len,
                             bool aAllowPartialMatch);
    nsresult ParseLine(char *line);
    nsresult ParseLineSegment(char *seg, PRUint32 len);
    nsresult ParseHead(char *, PRUint32 count, PRUint32 *countRead);
    nsresult HandleContentStart();
    nsresult HandleContent(char *, PRUint32 count, PRUint32 *contentRead, PRUint32 *contentRemaining);
    nsresult ProcessData(char *, PRUint32, PRUint32 *);
    void     DeleteSelfOnConsumerThread();

    Classifier Classify();
    void       CancelPipeline(PRUint32 reason);

    static NS_METHOD ReadRequestSegment(nsIInputStream *, void *, const char *,
                                        PRUint32, PRUint32, PRUint32 *);
    static NS_METHOD WritePipeSegment(nsIOutputStream *, void *, char *,
                                      PRUint32, PRUint32, PRUint32 *);

    bool TimingEnabled() const { return mCaps & NS_HTTP_TIMING_ENABLED; }

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
    PRUint64                        mRequestSize;

    nsAHttpConnection              *mConnection;      
    nsHttpConnectionInfo           *mConnInfo;        
    nsHttpRequestHead              *mRequestHead;     
    nsHttpResponseHead             *mResponseHead;    

    nsAHttpSegmentReader           *mReader;
    nsAHttpSegmentWriter           *mWriter;

    nsCString                       mLineBuf;         

    PRInt64                         mContentLength;   
    PRInt64                         mContentRead;     

    
    
    
    
    
    PRUint32                        mInvalidResponseBytesRead;

    nsHttpChunkedDecoder           *mChunkedDecoder;

    TimingStruct                    mTimings;

    nsresult                        mStatus;

    PRInt16                         mPriority;

    PRUint16                        mRestartCount;        
    PRUint8                         mCaps;
    enum Classifier                 mClassification;
    PRInt32                         mPipelinePosition;
    PRInt64                         mMaxPipelineObjectSize;

    
    
    bool                            mClosed;
    bool                            mConnected;
    bool                            mHaveStatusLine;
    bool                            mHaveAllHeaders;
    bool                            mTransactionDone;
    bool                            mResponseIsComplete;
    bool                            mDidContentStart;
    bool                            mNoContent; 
    bool                            mSentData;
    bool                            mReceivedData;
    bool                            mStatusEventPending;
    bool                            mHasRequestBody;
    bool                            mSSLConnectFailed;
    bool                            mHttpResponseMatched;
    bool                            mPreserveStream;

    
    
    

    
    bool                            mReportedStart;
    bool                            mReportedResponseHeader;

    
    nsHttpResponseHead             *mForTakeResponseHead;
    bool                            mResponseHeadTaken;

    class RestartVerifier 
    {

        
        
        
        
        
        
        

    public:
        RestartVerifier()
            : mContentLength(-1)
            , mAlreadyProcessed(0)
            , mToReadBeforeRestart(0)
            , mSetup(false)
        {}
        ~RestartVerifier() {}
        
        void Set(PRInt64 contentLength, nsHttpResponseHead *head);
        bool Verify(PRInt64 contentLength, nsHttpResponseHead *head);
        bool IsDiscardingContent() { return mToReadBeforeRestart != 0; }
        bool IsSetup() { return mSetup; }
        PRInt64 AlreadyProcessed() { return mAlreadyProcessed; }
        void SetAlreadyProcessed(PRInt64 val) {
            mAlreadyProcessed = val;
            mToReadBeforeRestart = val;
        }
        PRInt64 ToReadBeforeRestart() { return mToReadBeforeRestart; }
        void HaveReadBeforeRestart(PRUint32 amt)
        {
            NS_ABORT_IF_FALSE(amt <= mToReadBeforeRestart,
                              "too large of a HaveReadBeforeRestart deduction");
            mToReadBeforeRestart -= amt;
        }

    private:
        
        

        PRInt64                         mContentLength;
        nsCString                       mETag;
        nsCString                       mLastModified;
        nsCString                       mContentRange;
        nsCString                       mContentEncoding;
        nsCString                       mTransferEncoding;

        
        
        
        PRInt64                         mAlreadyProcessed;

        
        
        
        PRInt64                         mToReadBeforeRestart;

        
        bool                            mSetup;
    } mRestartInProgressVerifier;
};

#endif 
