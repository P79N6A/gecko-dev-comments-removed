




#ifndef nsHttpTransaction_h__
#define nsHttpTransaction_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "EventTokenBucket.h"
#include "nsCOMPtr.h"

#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsILoadGroup.h"
#include "nsIOutputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsISocketTransportService.h"
#include "nsITransport.h"
#include "nsIEventTarget.h"
#include "TimingStruct.h"



class nsHttpRequestHead;
class nsHttpResponseHead;
class nsHttpChunkedDecoder;
class nsIHttpActivityObserver;
class UpdateSecurityCallbacks;






class nsHttpTransaction : public nsAHttpTransaction
                        , public mozilla::net::ATokenBucketEvent
                        , public nsIInputStreamCallback
                        , public nsIOutputStreamCallback
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSAHTTPTRANSACTION
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSIOUTPUTSTREAMCALLBACK

    nsHttpTransaction();
    virtual ~nsHttpTransaction();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsresult Init(uint32_t               caps,
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

    nsIEventTarget        *ConsumerTarget() { return mConsumerTarget; }

    void SetSecurityCallbacks(nsIInterfaceRequestor* aCallbacks);

    
    
    nsHttpResponseHead *TakeResponseHead();

    
    bool ResponseIsComplete() { return mResponseIsComplete; }

    bool      ProxyConnectFailed() { return mProxyConnectFailed; }

    
    void    SetPriority(int32_t priority) { mPriority = priority; }
    int32_t    Priority()                 { return mPriority; }

    const TimingStruct& Timings() const { return mTimings; }
    enum Classifier Classification() { return mClassification; }

    void PrintDiagnostics(nsCString &log);

    
    void SetPendingTime(bool now = true) { mPendingTime = now ? mozilla::TimeStamp::Now() : mozilla::TimeStamp(); }
    const mozilla::TimeStamp GetPendingTime() { return mPendingTime; }
    bool UsesPipelining() const { return mCaps & NS_HTTP_ALLOW_PIPELINING; }

    
    nsILoadGroupConnectionInfo *LoadGroupConnectionInfo() { return mLoadGroupCI.get(); }
    void SetLoadGroupConnectionInfo(nsILoadGroupConnectionInfo *aLoadGroupCI) { mLoadGroupCI = aLoadGroupCI; }
    void DispatchedAsBlocking();
    void RemoveDispatchedAsBlocking();

private:
    nsresult Restart();
    nsresult RestartInProgress();
    char    *LocateHttpStart(char *buf, uint32_t len,
                             bool aAllowPartialMatch);
    nsresult ParseLine(char *line);
    nsresult ParseLineSegment(char *seg, uint32_t len);
    nsresult ParseHead(char *, uint32_t count, uint32_t *countRead);
    nsresult HandleContentStart();
    nsresult HandleContent(char *, uint32_t count, uint32_t *contentRead, uint32_t *contentRemaining);
    nsresult ProcessData(char *, uint32_t, uint32_t *);
    void     DeleteSelfOnConsumerThread();
    void     ReleaseBlockingTransaction();

    Classifier Classify();
    void       CancelPipeline(uint32_t reason);

    static NS_METHOD ReadRequestSegment(nsIInputStream *, void *, const char *,
                                        uint32_t, uint32_t, uint32_t *);
    static NS_METHOD WritePipeSegment(nsIOutputStream *, void *, char *,
                                      uint32_t, uint32_t, uint32_t *);

    bool TimingEnabled() const { return mCaps & NS_HTTP_TIMING_ENABLED; }

private:
    class UpdateSecurityCallbacks : public nsRunnable
    {
      public:
        UpdateSecurityCallbacks(nsHttpTransaction* aTrans,
                                nsIInterfaceRequestor* aCallbacks)
        : mTrans(aTrans), mCallbacks(aCallbacks) {}

        NS_IMETHOD Run()
        {
            if (mTrans->mConnection)
                mTrans->mConnection->SetSecurityCallbacks(mCallbacks);
            return NS_OK;
        }
      private:
        nsRefPtr<nsHttpTransaction> mTrans;
        nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    };

    mozilla::Mutex mCallbacksLock;

    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsITransportEventSink> mTransportSink;
    nsCOMPtr<nsIEventTarget>        mConsumerTarget;
    nsCOMPtr<nsISupports>           mSecurityInfo;
    nsCOMPtr<nsIAsyncInputStream>   mPipeIn;
    nsCOMPtr<nsIAsyncOutputStream>  mPipeOut;
    nsCOMPtr<nsILoadGroupConnectionInfo> mLoadGroupCI;

    nsCOMPtr<nsISupports>             mChannel;
    nsCOMPtr<nsIHttpActivityObserver> mActivityDistributor;

    nsCString                       mReqHeaderBuf;    
    nsCOMPtr<nsIInputStream>        mRequestStream;
    uint64_t                        mRequestSize;

    nsAHttpConnection              *mConnection;      
    nsHttpConnectionInfo           *mConnInfo;        
    nsHttpRequestHead              *mRequestHead;     
    nsHttpResponseHead             *mResponseHead;    

    nsAHttpSegmentReader           *mReader;
    nsAHttpSegmentWriter           *mWriter;

    nsCString                       mLineBuf;         

    int64_t                         mContentLength;   
    int64_t                         mContentRead;     

    
    
    
    
    
    uint32_t                        mInvalidResponseBytesRead;

    nsHttpChunkedDecoder           *mChunkedDecoder;

    TimingStruct                    mTimings;

    nsresult                        mStatus;

    int16_t                         mPriority;

    uint16_t                        mRestartCount;        
    uint32_t                        mCaps;
    enum Classifier                 mClassification;
    int32_t                         mPipelinePosition;
    int64_t                         mMaxPipelineObjectSize;

    nsHttpVersion                   mHttpVersion;

    
    
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
    bool                            mProxyConnectFailed;
    bool                            mHttpResponseMatched;
    bool                            mPreserveStream;
    bool                            mDispatchedAsBlocking;

    
    
    

    
    bool                            mReportedStart;
    bool                            mReportedResponseHeader;

    
    nsHttpResponseHead             *mForTakeResponseHead;
    bool                            mResponseHeadTaken;

    
    mozilla::TimeStamp              mPendingTime;

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

        void Set(int64_t contentLength, nsHttpResponseHead *head);
        bool Verify(int64_t contentLength, nsHttpResponseHead *head);
        bool IsDiscardingContent() { return mToReadBeforeRestart != 0; }
        bool IsSetup() { return mSetup; }
        int64_t AlreadyProcessed() { return mAlreadyProcessed; }
        void SetAlreadyProcessed(int64_t val) {
            mAlreadyProcessed = val;
            mToReadBeforeRestart = val;
        }
        int64_t ToReadBeforeRestart() { return mToReadBeforeRestart; }
        void HaveReadBeforeRestart(uint32_t amt)
        {
            MOZ_ASSERT(amt <= mToReadBeforeRestart,
                       "too large of a HaveReadBeforeRestart deduction");
            mToReadBeforeRestart -= amt;
        }

    private:
        
        

        int64_t                         mContentLength;
        nsCString                       mETag;
        nsCString                       mLastModified;
        nsCString                       mContentRange;
        nsCString                       mContentEncoding;
        nsCString                       mTransferEncoding;

        
        
        
        int64_t                         mAlreadyProcessed;

        
        
        
        int64_t                         mToReadBeforeRestart;

        
        bool                            mSetup;
    } mRestartInProgressVerifier;


public:
    
    
    
    bool TryToRunPacedRequest();

    
    
    
    
    
    void OnTokenBucketAdmitted(); 

    
    
    
    
    void CancelPacing(nsresult reason);

private:
    bool mSubmittedRatePacing;
    bool mPassedRatePacing;
    bool mSynchronousRatePaceRequest;
    nsCOMPtr<nsICancelable> mTokenBucketCancel;
};

#endif 
