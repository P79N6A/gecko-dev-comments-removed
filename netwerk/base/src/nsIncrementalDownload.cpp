





































#include "nsIIncrementalDownload.h"
#include "nsIRequestObserver.h"
#include "nsIProgressEventSink.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsIPropertyBag2.h"
#include "nsIServiceManager.h"
#include "nsILocalFile.h"
#include "nsITimer.h"
#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "nsChannelProperties.h"
#include "prio.h"
#include "prprf.h"



#define NS_ERROR_DOWNLOAD_COMPLETE \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL, 1)



#define NS_ERROR_DOWNLOAD_NOT_PARTIAL \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL, 2)


#define DEFAULT_CHUNK_SIZE (4096 * 16)  // bytes
#define DEFAULT_INTERVAL    60          // seconds

#define UPDATE_PROGRESS_INTERVAL PRTime(500 * PR_USEC_PER_MSEC) // 500ms


#define MAX_RETRY_COUNT 20



static nsresult
WriteToFile(nsILocalFile *lf, const char *data, PRUint32 len, PRInt32 flags)
{
  PRFileDesc *fd;
  nsresult rv = lf->OpenNSPRFileDesc(flags, 0600, &fd);
  if (NS_FAILED(rv))
    return rv;

  if (len)
    rv = PR_Write(fd, data, len) == PRInt32(len) ? NS_OK : NS_ERROR_FAILURE;

  PR_Close(fd);
  return rv;
}

static nsresult
AppendToFile(nsILocalFile *lf, const char *data, PRUint32 len)
{
  PRInt32 flags = PR_WRONLY | PR_CREATE_FILE | PR_APPEND;
  return WriteToFile(lf, data, len, flags);
}


static void
MakeRangeSpec(const PRInt64 &size, const PRInt64 &maxSize, PRInt32 chunkSize,
              PRBool fetchRemaining, nsCString &rangeSpec)
{
  rangeSpec.AssignLiteral("bytes=");
  rangeSpec.AppendInt(PRInt64(size));
  rangeSpec.Append('-');

  if (fetchRemaining)
    return;

  PRInt64 end = size + PRInt64(chunkSize);
  if (maxSize != PRInt64(-1) && end > maxSize)
    end = maxSize;
  end -= 1;

  rangeSpec.AppendInt(PRInt64(end));
}



class nsIncrementalDownload : public nsIIncrementalDownload
                            , public nsIStreamListener
                            , public nsIObserver
                            , public nsIInterfaceRequestor
                            , public nsIChannelEventSink
                            , public nsSupportsWeakReference
                            , public nsIAsyncVerifyRedirectCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSIINCREMENTALDOWNLOAD
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

  nsIncrementalDownload();

private:
  ~nsIncrementalDownload() {}
  nsresult FlushChunk();
  void     UpdateProgress();
  nsresult CallOnStartRequest();
  void     CallOnStopRequest();
  nsresult StartTimer(PRInt32 interval);
  nsresult ProcessTimeout();
  nsresult ReadCurrentSize();
  nsresult ClearRequestHeader(nsIHttpChannel *channel);

  nsCOMPtr<nsIRequestObserver>             mObserver;
  nsCOMPtr<nsISupports>                    mObserverContext;
  nsCOMPtr<nsIProgressEventSink>           mProgressSink;
  nsCOMPtr<nsIURI>                         mURI;
  nsCOMPtr<nsIURI>                         mFinalURI;
  nsCOMPtr<nsILocalFile>                   mDest;
  nsCOMPtr<nsIChannel>                     mChannel;
  nsCOMPtr<nsITimer>                       mTimer;
  nsAutoArrayPtr<char>                     mChunk;
  PRInt32                                  mChunkLen;
  PRInt32                                  mChunkSize;
  PRInt32                                  mInterval;
  PRInt64                                  mTotalSize;
  PRInt64                                  mCurrentSize;
  PRUint32                                 mLoadFlags;
  PRInt32                                  mNonPartialCount;
  nsresult                                 mStatus;
  PRPackedBool                             mIsPending;
  PRPackedBool                             mDidOnStartRequest;
  PRTime                                   mLastProgressUpdate;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel>                     mNewRedirectChannel;
};

nsIncrementalDownload::nsIncrementalDownload()
  : mChunkLen(0)
  , mChunkSize(DEFAULT_CHUNK_SIZE)
  , mInterval(DEFAULT_INTERVAL)
  , mTotalSize(-1)
  , mCurrentSize(-1)
  , mLoadFlags(LOAD_NORMAL)
  , mNonPartialCount(0)
  , mStatus(NS_OK)
  , mIsPending(PR_FALSE)
  , mDidOnStartRequest(PR_FALSE)
  , mLastProgressUpdate(0)
  , mRedirectCallback(nsnull)
  , mNewRedirectChannel(nsnull)
{
}

nsresult
nsIncrementalDownload::FlushChunk()
{
  NS_ASSERTION(mTotalSize != PRInt64(-1), "total size should be known");

  if (mChunkLen == 0)
    return NS_OK;

  nsresult rv = AppendToFile(mDest, mChunk, mChunkLen);
  if (NS_FAILED(rv))
    return rv;

  mCurrentSize += PRInt64(mChunkLen);
  mChunkLen = 0;

  return NS_OK;
}

void
nsIncrementalDownload::UpdateProgress()
{
  mLastProgressUpdate = PR_Now();

  if (mProgressSink)
    mProgressSink->OnProgress(this, mObserverContext,
                              PRUint64(PRInt64(mCurrentSize) + mChunkLen),
                              PRUint64(PRInt64(mTotalSize)));
}

nsresult
nsIncrementalDownload::CallOnStartRequest()
{
  if (!mObserver || mDidOnStartRequest)
    return NS_OK;

  mDidOnStartRequest = PR_TRUE;
  return mObserver->OnStartRequest(this, mObserverContext);
}

void
nsIncrementalDownload::CallOnStopRequest()
{
  if (!mObserver)
    return;

  
  nsresult rv = CallOnStartRequest();
  if (NS_SUCCEEDED(mStatus))
    mStatus = rv;

  mIsPending = PR_FALSE;

  mObserver->OnStopRequest(this, mObserverContext, mStatus);
  mObserver = nsnull;
  mObserverContext = nsnull;
}

nsresult
nsIncrementalDownload::StartTimer(PRInt32 interval)
{
  nsresult rv;
  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  return mTimer->Init(this, interval * 1000, nsITimer::TYPE_ONE_SHOT);
}

nsresult
nsIncrementalDownload::ProcessTimeout()
{
  NS_ASSERTION(!mChannel, "how can we have a channel?");

  
  if (NS_FAILED(mStatus)) {
    CallOnStopRequest();
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = NS_NewChannel(getter_AddRefs(channel), mFinalURI, nsnull,
                              nsnull, this, mLoadFlags);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(channel, &rv);
  if (NS_FAILED(rv))
    return rv;

  NS_ASSERTION(mCurrentSize != PRInt64(-1),
      "we should know the current file size by now");

  rv = ClearRequestHeader(http);
  if (NS_FAILED(rv))
    return rv;

  
  
  if (mInterval || mCurrentSize != PRInt64(0)) {
    nsCAutoString range;
    MakeRangeSpec(mCurrentSize, mTotalSize, mChunkSize, mInterval == 0, range);

    rv = http->SetRequestHeader(NS_LITERAL_CSTRING("Range"), range, PR_FALSE);
    if (NS_FAILED(rv))
      return rv;
  }

  rv = channel->AsyncOpen(this, nsnull);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  mChannel = channel;
  return NS_OK;
}


nsresult
nsIncrementalDownload::ReadCurrentSize()
{
  PRInt64 size;
  nsresult rv = mDest->GetFileSize((PRInt64 *) &size);
  if (rv == NS_ERROR_FILE_NOT_FOUND ||
      rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
    mCurrentSize = 0;
    return NS_OK;
  }
  if (NS_FAILED(rv))
    return rv;

  mCurrentSize = size; 
  return NS_OK;
}



NS_IMPL_ISUPPORTS9(nsIncrementalDownload,
                   nsIIncrementalDownload,
                   nsIRequest,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIObserver,
                   nsIInterfaceRequestor,
                   nsIChannelEventSink,
                   nsISupportsWeakReference,
                   nsIAsyncVerifyRedirectCallback)



NS_IMETHODIMP
nsIncrementalDownload::GetName(nsACString &name)
{
  NS_ENSURE_TRUE(mURI, NS_ERROR_NOT_INITIALIZED);

  return mURI->GetSpec(name);
}

NS_IMETHODIMP
nsIncrementalDownload::IsPending(PRBool *isPending)
{
  *isPending = mIsPending;
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::GetStatus(nsresult *status)
{
  *status = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::Cancel(nsresult status)
{
  NS_ENSURE_ARG(NS_FAILED(status));

  
  if (NS_FAILED(mStatus))
    return NS_OK;

  mStatus = status;

  
  if (!mIsPending)
    return NS_OK;

  if (mChannel) {
    mChannel->Cancel(mStatus);
    NS_ASSERTION(!mTimer, "what is this timer object doing here?");
  }
  else {
    
    
    if (mTimer)
      mTimer->Cancel();
    StartTimer(0);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::Suspend()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIncrementalDownload::Resume()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIncrementalDownload::GetLoadFlags(nsLoadFlags *loadFlags)
{
  *loadFlags = mLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::SetLoadFlags(nsLoadFlags loadFlags)
{
  mLoadFlags = loadFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::GetLoadGroup(nsILoadGroup **loadGroup)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIncrementalDownload::SetLoadGroup(nsILoadGroup *loadGroup)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
nsIncrementalDownload::Init(nsIURI *uri, nsIFile *dest,
                            PRInt32 chunkSize, PRInt32 interval)
{
  
  NS_ENSURE_FALSE(mURI, NS_ERROR_ALREADY_INITIALIZED);

  mDest = do_QueryInterface(dest);
  NS_ENSURE_ARG(mDest);

  mURI = uri;
  mFinalURI = uri;

  if (chunkSize > 0)
    mChunkSize = chunkSize;
  if (interval >= 0)
    mInterval = interval;
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::GetURI(nsIURI **result)
{
  NS_IF_ADDREF(*result = mURI);
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::GetFinalURI(nsIURI **result)
{
  NS_IF_ADDREF(*result = mFinalURI);
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::GetDestination(nsIFile **result)
{
  if (!mDest) {
    *result = nsnull;
    return NS_OK;
  }
  
  
  
  return mDest->Clone(result);
}

NS_IMETHODIMP
nsIncrementalDownload::GetTotalSize(PRInt64 *result)
{
  *result = mTotalSize;
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::GetCurrentSize(PRInt64 *result)
{
  *result = mCurrentSize;
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::Start(nsIRequestObserver *observer,
                             nsISupports *context)
{
  NS_ENSURE_ARG(observer);
  NS_ENSURE_FALSE(mIsPending, NS_ERROR_IN_PROGRESS);

  
  
  
  
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs)
    obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);

  nsresult rv = ReadCurrentSize();
  if (NS_FAILED(rv))
    return rv;

  rv = StartTimer(0);
  if (NS_FAILED(rv))
    return rv;

  mObserver = observer;
  mObserverContext = context;
  mProgressSink = do_QueryInterface(observer);  

  mIsPending = PR_TRUE;
  return NS_OK;
}



NS_IMETHODIMP
nsIncrementalDownload::OnStartRequest(nsIRequest *request,
                                      nsISupports *context)
{
  nsresult rv;

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(request, &rv);
  if (NS_FAILED(rv))
    return rv;

  
  PRUint32 code;
  rv = http->GetResponseStatus(&code);
  if (NS_FAILED(rv))
    return rv;
  if (code != 206) {
    
    
    
    if (code == 416 && mTotalSize == PRInt64(-1)) {
      mTotalSize = mCurrentSize;
      
      return NS_ERROR_DOWNLOAD_COMPLETE;
    }
    
    
    
    
    if (code == 200) {
      if (mInterval) {
        mChannel = nsnull;
        if (++mNonPartialCount > MAX_RETRY_COUNT) {
          NS_WARNING("unable to fetch a byte range; giving up");
          return NS_ERROR_FAILURE;
        }
        
        StartTimer(mInterval * mNonPartialCount);
        return NS_ERROR_DOWNLOAD_NOT_PARTIAL;
      }
      
      
      
    } else {
      NS_WARNING("server response was unexpected");
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    
    
    mNonPartialCount = 0;
  }

  
  if (mTotalSize == PRInt64(-1)) {
    
    rv = http->GetURI(getter_AddRefs(mFinalURI));
    if (NS_FAILED(rv))
      return rv;

    if (code == 206) {
      
      
      nsCAutoString buf;
      rv = http->GetResponseHeader(NS_LITERAL_CSTRING("Content-Range"), buf);
      if (NS_FAILED(rv))
        return rv;
      PRInt32 slash = buf.FindChar('/');
      if (slash == kNotFound) {
        NS_WARNING("server returned invalid Content-Range header!");
        return NS_ERROR_UNEXPECTED;
      }
      if (PR_sscanf(buf.get() + slash + 1, "%lld", (PRInt64 *) &mTotalSize) != 1)
        return NS_ERROR_UNEXPECTED;
    } else {
      
      
      nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(request, &rv);
      if (NS_FAILED(rv))
        return rv;
      rv = props->GetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH,
                                     &mTotalSize);
      
      if (mTotalSize == PRInt64(-1)) {
        NS_WARNING("server returned no content-length header!");
        return NS_ERROR_UNEXPECTED;
      }
      
      
      WriteToFile(mDest, nsnull, 0, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE);
      mCurrentSize = 0;
    }

    
    rv = CallOnStartRequest();
    if (NS_FAILED(rv))
      return rv;
  }

  
  PRInt64 diff = mTotalSize - mCurrentSize;
  if (diff <= PRInt64(0)) {
    NS_WARNING("about to set a bogus chunk size; giving up");
    return NS_ERROR_UNEXPECTED;
  }

  if (diff < PRInt64(mChunkSize))
    mChunkSize = PRUint32(diff);

  mChunk = new char[mChunkSize];
  if (!mChunk)
    rv = NS_ERROR_OUT_OF_MEMORY;

  return rv;
}

NS_IMETHODIMP
nsIncrementalDownload::OnStopRequest(nsIRequest *request,
                                     nsISupports *context,
                                     nsresult status)
{
  
  
  if (status == NS_ERROR_DOWNLOAD_NOT_PARTIAL)
    return NS_OK;

  
  if (status == NS_ERROR_DOWNLOAD_COMPLETE)
    status = NS_OK;

  if (NS_SUCCEEDED(mStatus))
    mStatus = status;

  if (mChunk) {
    if (NS_SUCCEEDED(mStatus))
      mStatus = FlushChunk();

    mChunk = nsnull;  
    mChunkLen = 0;
    UpdateProgress();
  }

  mChannel = nsnull;

  
  if (NS_FAILED(mStatus) || mCurrentSize == mTotalSize) {
    CallOnStopRequest();
    return NS_OK;
  }

  return StartTimer(mInterval);  
}



NS_IMETHODIMP
nsIncrementalDownload::OnDataAvailable(nsIRequest *request,
                                       nsISupports *context,
                                       nsIInputStream *input,
                                       PRUint32 offset,
                                       PRUint32 count)
{
  while (count) {
    PRUint32 space = mChunkSize - mChunkLen;
    PRUint32 n, len = NS_MIN(space, count);

    nsresult rv = input->Read(mChunk + mChunkLen, len, &n);
    if (NS_FAILED(rv))
      return rv;
    if (n != len)
      return NS_ERROR_UNEXPECTED;

    count -= n;
    mChunkLen += n;

    if (mChunkLen == mChunkSize)
      FlushChunk();
  }

  if (PR_Now() > mLastProgressUpdate + UPDATE_PROGRESS_INTERVAL)
    UpdateProgress();

  return NS_OK;
}



NS_IMETHODIMP
nsIncrementalDownload::Observe(nsISupports *subject, const char *topic,
                               const PRUnichar *data)
{
  if (strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Cancel(NS_ERROR_ABORT);

    
    
    
    CallOnStopRequest();
  }
  else if (strcmp(topic, NS_TIMER_CALLBACK_TOPIC) == 0) {
    mTimer = nsnull;
    nsresult rv = ProcessTimeout();
    if (NS_FAILED(rv))
      Cancel(rv);
  }
  return NS_OK;
}



NS_IMETHODIMP
nsIncrementalDownload::GetInterface(const nsIID &iid, void **result)
{
  if (iid.Equals(NS_GET_IID(nsIChannelEventSink))) {
    NS_ADDREF_THIS();
    *result = static_cast<nsIChannelEventSink *>(this);
    return NS_OK;
  }

  nsCOMPtr<nsIInterfaceRequestor> ir = do_QueryInterface(mObserver);
  if (ir)
    return ir->GetInterface(iid, result);

  return NS_ERROR_NO_INTERFACE;
}

nsresult 
nsIncrementalDownload::ClearRequestHeader(nsIHttpChannel *channel)
{
  NS_ENSURE_ARG(channel);
  
  
  
  return channel->SetRequestHeader(NS_LITERAL_CSTRING("Accept-Encoding"),
                                   NS_LITERAL_CSTRING(""), PR_FALSE);
}



NS_IMETHODIMP
nsIncrementalDownload::AsyncOnChannelRedirect(nsIChannel *oldChannel,
                                              nsIChannel *newChannel,
                                              PRUint32 flags,
                                              nsIAsyncVerifyRedirectCallback *cb)
{
  
  
 
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(oldChannel);
  NS_ENSURE_STATE(http);

  nsCOMPtr<nsIHttpChannel> newHttpChannel = do_QueryInterface(newChannel);
  NS_ENSURE_STATE(newHttpChannel);

  NS_NAMED_LITERAL_CSTRING(rangeHdr, "Range");

  nsresult rv = ClearRequestHeader(newHttpChannel);
  if (NS_FAILED(rv))
    return rv;

  
  nsCAutoString rangeVal;
  http->GetRequestHeader(rangeHdr, rangeVal);
  if (!rangeVal.IsEmpty()) {
    rv = newHttpChannel->SetRequestHeader(rangeHdr, rangeVal, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  mRedirectCallback = cb;
  mNewRedirectChannel = newChannel;

  
  nsCOMPtr<nsIChannelEventSink> sink = do_GetInterface(mObserver);
  if (sink) {
    rv = sink->AsyncOnChannelRedirect(oldChannel, newChannel, flags, this);
    if (NS_FAILED(rv)) {
        mRedirectCallback = nsnull;
        mNewRedirectChannel = nsnull;
    }
    return rv;
  }
  (void) OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

NS_IMETHODIMP
nsIncrementalDownload::OnRedirectVerifyCallback(nsresult result)
{
  NS_ASSERTION(mRedirectCallback, "mRedirectCallback not set in callback");
  NS_ASSERTION(mNewRedirectChannel, "mNewRedirectChannel not set in callback");

  
  if (NS_SUCCEEDED(result))
    mChannel = mNewRedirectChannel;

  mRedirectCallback->OnRedirectVerifyCallback(result);
  mRedirectCallback = nsnull;
  mNewRedirectChannel = nsnull;
  return NS_OK;
}

extern nsresult
net_NewIncrementalDownload(nsISupports *outer, const nsIID &iid, void **result)
{
  if (outer)
    return NS_ERROR_NO_AGGREGATION;

  nsIncrementalDownload *d = new nsIncrementalDownload();
  if (!d)
    return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(d);
  nsresult rv = d->QueryInterface(iid, result);
  NS_RELEASE(d);
  return rv;
}
