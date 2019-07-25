





































#include "nsMediaStream.h"

#include "mozilla/Mutex.h"
#include "nsDebug.h"
#include "nsMediaDecoder.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsIFile.h"
#include "nsIFileChannel.h"
#include "nsIHttpChannel.h"
#include "nsISeekableStream.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsIScriptSecurityManager.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsHTMLMediaElement.h"
#include "nsIDocument.h"
#include "nsDOMError.h"
#include "nsICachingChannel.h"
#include "nsURILoader.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "mozilla/Util.h" 
#include "nsContentUtils.h"

static const PRUint32 HTTP_OK_CODE = 200;
static const PRUint32 HTTP_PARTIAL_RESPONSE_CODE = 206;

using namespace mozilla;

nsMediaChannelStream::nsMediaChannelStream(nsMediaDecoder* aDecoder,
    nsIChannel* aChannel, nsIURI* aURI)
  : nsMediaStream(aDecoder, aChannel, aURI),
    mOffset(0), mSuspendCount(0),
    mReopenOnError(false), mIgnoreClose(false),
    mCacheStream(this),
    mLock("nsMediaChannelStream.mLock"),
    mIgnoreResume(false)
{
}

nsMediaChannelStream::~nsMediaChannelStream()
{
  if (mListener) {
    
    mListener->Revoke();
  }
}







NS_IMPL_ISUPPORTS4(nsMediaChannelStream::Listener,
                   nsIRequestObserver, nsIStreamListener, nsIChannelEventSink,
                   nsIInterfaceRequestor)

nsresult
nsMediaChannelStream::Listener::OnStartRequest(nsIRequest* aRequest,
                                               nsISupports* aContext)
{
  if (!mStream)
    return NS_OK;
  return mStream->OnStartRequest(aRequest);
}

nsresult
nsMediaChannelStream::Listener::OnStopRequest(nsIRequest* aRequest,
                                              nsISupports* aContext,
                                              nsresult aStatus)
{
  if (!mStream)
    return NS_OK;
  return mStream->OnStopRequest(aRequest, aStatus);
}

nsresult
nsMediaChannelStream::Listener::OnDataAvailable(nsIRequest* aRequest,
                                                nsISupports* aContext,
                                                nsIInputStream* aStream,
                                                PRUint32 aOffset,
                                                PRUint32 aCount)
{
  if (!mStream)
    return NS_OK;
  return mStream->OnDataAvailable(aRequest, aStream, aCount);
}

nsresult
nsMediaChannelStream::Listener::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                                       nsIChannel* aNewChannel,
                                                       PRUint32 aFlags,
                                                       nsIAsyncVerifyRedirectCallback* cb)
{
  nsresult rv = NS_OK;
  if (mStream)
    rv = mStream->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);

  if (NS_FAILED(rv))
    return rv;

  cb->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

nsresult
nsMediaChannelStream::Listener::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}

nsresult
nsMediaChannelStream::OnStartRequest(nsIRequest* aRequest)
{
  NS_ASSERTION(mChannel.get() == aRequest, "Wrong channel!");

  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if (element->ShouldCheckAllowOrigin()) {
    
    
    if (status == NS_ERROR_DOM_BAD_URI) {
      mDecoder->NetworkError();
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(aRequest);
  bool seekable = false;
  if (hc) {
    PRUint32 responseStatus = 0;
    hc->GetResponseStatus(&responseStatus);
    bool succeeded = false;
    hc->GetRequestSucceeded(&succeeded);

    if (!succeeded && NS_SUCCEEDED(status)) {
      
      
      
      
      
      
      
      
      
      if (responseStatus != HTTP_REQUESTED_RANGE_NOT_SATISFIABLE_CODE) {
        mDecoder->NetworkError();
      }

      
      
      CloseChannel();
      return NS_OK;
    }

    nsCAutoString ranges;
    hc->GetResponseHeader(NS_LITERAL_CSTRING("Accept-Ranges"),
                          ranges);
    bool acceptsRanges = ranges.EqualsLiteral("bytes");

    if (mOffset == 0) {
      
      
      
      
      
      
      
      
      nsCAutoString durationText;
      PRInt32 ec = 0;
      rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("Content-Duration"), durationText);
      if (NS_FAILED(rv)) {
        rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-AMZ-Meta-Content-Duration"), durationText);
      }
      if (NS_FAILED(rv)) {
        rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-Content-Duration"), durationText);
      }

      if (NS_SUCCEEDED(rv)) {
        double duration = durationText.ToDouble(&ec);
        if (ec == NS_OK && duration >= 0) {
          mDecoder->SetDuration(duration);
        }
      } else {
        mDecoder->SetInfinite(true);
      }
    }

    if (mOffset > 0 && responseStatus == HTTP_OK_CODE) {
      
      
      
      mCacheStream.NotifyDataStarted(0);
      mOffset = 0;

      
      acceptsRanges = false;
    } else if (mOffset == 0 &&
               (responseStatus == HTTP_OK_CODE ||
                responseStatus == HTTP_PARTIAL_RESPONSE_CODE)) {
      
      
      PRInt32 cl = -1;
      hc->GetContentLength(&cl);
      if (cl >= 0) {
        mCacheStream.NotifyDataLength(cl);
      }
    }
    
    

    
    
    
    seekable =
      responseStatus == HTTP_PARTIAL_RESPONSE_CODE || acceptsRanges;

    if (seekable) {
      mDecoder->SetInfinite(false);
    }
  }
  mDecoder->SetSeekable(seekable);
  mCacheStream.SetSeekable(seekable);

  nsCOMPtr<nsICachingChannel> cc = do_QueryInterface(aRequest);
  if (cc) {
    bool fromCache = false;
    rv = cc->IsFromCache(&fromCache);
    if (NS_SUCCEEDED(rv) && !fromCache) {
      cc->SetCacheAsFile(true);
    }
  }

  {
    MutexAutoLock lock(mLock);
    mChannelStatistics.Start(TimeStamp::Now());
  }

  mReopenOnError = false;
  mIgnoreClose = false;
  if (mSuspendCount > 0) {
    
    
    
    mChannel->Suspend();
    mIgnoreResume = false;
  }

  
  
  mDecoder->Progress(false);

  return NS_OK;
}

nsresult
nsMediaChannelStream::OnStopRequest(nsIRequest* aRequest, nsresult aStatus)
{
  NS_ASSERTION(mChannel.get() == aRequest, "Wrong channel!");
  NS_ASSERTION(mSuspendCount == 0,
               "How can OnStopRequest fire while we're suspended?");

  {
    MutexAutoLock lock(mLock);
    mChannelStatistics.Stop(TimeStamp::Now());
  }

  
  
  
  
  
  
  if (mReopenOnError &&
      aStatus != NS_ERROR_PARSED_DATA_CACHED && aStatus != NS_BINDING_ABORTED &&
      (mOffset == 0 || mCacheStream.IsSeekable())) {
    
    
    
    nsresult rv = CacheClientSeek(mOffset, false);
    if (NS_SUCCEEDED(rv))
      return rv;
    
    
  }

  if (!mIgnoreClose) {
    mCacheStream.NotifyDataEnded(aStatus);

    
    
    
    if (mLoadInBackground) {
      mLoadInBackground = false;

      nsLoadFlags loadFlags;
      DebugOnly<nsresult> rv = mChannel->GetLoadFlags(&loadFlags);
      NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadFlags() failed!");

      loadFlags &= ~nsIRequest::LOAD_BACKGROUND;
      ModifyLoadFlags(loadFlags);
    }
  }

  return NS_OK;
}

nsresult
nsMediaChannelStream::OnChannelRedirect(nsIChannel* aOld, nsIChannel* aNew,
                                        PRUint32 aFlags)
{
  mChannel = aNew;
  SetupChannelHeaders();
  return NS_OK;
}

struct CopySegmentClosure {
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsMediaChannelStream*  mStream;
};

NS_METHOD
nsMediaChannelStream::CopySegmentToCache(nsIInputStream *aInStream,
                                         void *aClosure,
                                         const char *aFromSegment,
                                         PRUint32 aToOffset,
                                         PRUint32 aCount,
                                         PRUint32 *aWriteCount)
{
  CopySegmentClosure* closure = static_cast<CopySegmentClosure*>(aClosure);

  closure->mStream->mDecoder->NotifyDataArrived(aFromSegment, aCount, closure->mStream->mOffset);

  
  closure->mStream->mOffset += aCount;
  closure->mStream->mCacheStream.NotifyDataReceived(aCount, aFromSegment,
                                                    closure->mPrincipal);
  *aWriteCount = aCount;
  return NS_OK;
}

nsresult
nsMediaChannelStream::OnDataAvailable(nsIRequest* aRequest,
                                      nsIInputStream* aStream,
                                      PRUint32 aCount)
{
  NS_ASSERTION(mChannel.get() == aRequest, "Wrong channel!");

  {
    MutexAutoLock lock(mLock);
    mChannelStatistics.AddBytes(aCount);
  }

  CopySegmentClosure closure;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (secMan && mChannel) {
    secMan->GetChannelPrincipal(mChannel, getter_AddRefs(closure.mPrincipal));
  }
  closure.mStream = this;

  PRUint32 count = aCount;
  while (count > 0) {
    PRUint32 read;
    nsresult rv = aStream->ReadSegments(CopySegmentToCache, &closure, count,
                                        &read);
    if (NS_FAILED(rv))
      return rv;
    NS_ASSERTION(read > 0, "Read 0 bytes while data was available?");
    count -= read;
  }

  return NS_OK;
}

nsresult nsMediaChannelStream::Open(nsIStreamListener **aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsresult rv = mCacheStream.Init();
  if (NS_FAILED(rv))
    return rv;
  NS_ASSERTION(mOffset == 0, "Who set mOffset already?");

  if (!mChannel) {
    
    
    NS_ASSERTION(!aStreamListener,
                 "Should have already been given a channel if we're to return a stream listener");
    return NS_OK;
  }

  return OpenChannel(aStreamListener);
}

nsresult nsMediaChannelStream::OpenChannel(nsIStreamListener** aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ENSURE_TRUE(mChannel, NS_ERROR_NULL_POINTER);
  NS_ASSERTION(!mListener, "Listener should have been removed by now");

  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  mListener = new Listener(this);
  NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);

  if (aStreamListener) {
    *aStreamListener = mListener;
    NS_ADDREF(*aStreamListener);
  } else {
    mChannel->SetNotificationCallbacks(mListener.get());

    nsCOMPtr<nsIStreamListener> listener = mListener.get();

    
    
    nsHTMLMediaElement* element = mDecoder->GetMediaElement();
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
    if (element->ShouldCheckAllowOrigin()) {
      nsresult rv;
      nsCORSListenerProxy* crossSiteListener =
        new nsCORSListenerProxy(mListener,
                                element->NodePrincipal(),
                                mChannel,
                                false,
                                &rv);
      listener = crossSiteListener;
      NS_ENSURE_TRUE(crossSiteListener, NS_ERROR_OUT_OF_MEMORY);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      nsresult rv = nsContentUtils::GetSecurityManager()->
        CheckLoadURIWithPrincipal(element->NodePrincipal(),
                                  mURI,
                                  nsIScriptSecurityManager::STANDARD);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    SetupChannelHeaders();

    nsresult rv = mChannel->AsyncOpen(listener, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void nsMediaChannelStream::SetupChannelHeaders()
{
  
  
  
  
  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(mChannel);
  if (hc) {
    nsCAutoString rangeString("bytes=");
    rangeString.AppendInt(mOffset);
    rangeString.Append("-");
    hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString, false);

    
    NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
    nsHTMLMediaElement* element = mDecoder->GetMediaElement();
    if (!element) {
      return;
    }
    element->SetRequestHeaders(hc);
  } else {
    NS_ASSERTION(mOffset == 0, "Don't know how to seek on this channel type");
  }
}

nsresult nsMediaChannelStream::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  mCacheStream.Close();
  CloseChannel();
  return NS_OK;
}

already_AddRefed<nsIPrincipal> nsMediaChannelStream::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIPrincipal> principal = mCacheStream.GetCurrentPrincipal();
  return principal.forget();
}

nsMediaStream* nsMediaChannelStream::CloneData(nsMediaDecoder* aDecoder)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsMediaChannelStream* stream = new nsMediaChannelStream(aDecoder, nsnull, mURI);
  if (stream) {
    
    
    
    
    
    
    stream->mSuspendCount = 1;
    stream->mCacheStream.InitAsClone(&mCacheStream);
    stream->mChannelStatistics = mChannelStatistics;
    stream->mChannelStatistics.Stop(TimeStamp::Now());
  }
  return stream;
}

void nsMediaChannelStream::CloseChannel()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  {
    MutexAutoLock lock(mLock);
    mChannelStatistics.Stop(TimeStamp::Now());
  }

  if (mListener) {
    mListener->Revoke();
    mListener = nsnull;
  }

  if (mChannel) {
    if (mSuspendCount > 0) {
      
      PossiblyResume();
    }
    
    
    
    
    
    
    
    mChannel->Cancel(NS_ERROR_PARSED_DATA_CACHED);
    mChannel = nsnull;
  }
}

nsresult nsMediaChannelStream::ReadFromCache(char* aBuffer,
                                             PRInt64 aOffset,
                                             PRUint32 aCount)
{
  return mCacheStream.ReadFromCache(aBuffer, aOffset, aCount);
}

nsresult nsMediaChannelStream::Read(char* aBuffer,
                                    PRUint32 aCount,
                                    PRUint32* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  return mCacheStream.Read(aBuffer, aCount, aBytes);
}

nsresult nsMediaChannelStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  return mCacheStream.Seek(aWhence, aOffset);
}

PRInt64 nsMediaChannelStream::Tell()
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  return mCacheStream.Tell();
}

nsresult nsMediaChannelStream::GetCachedRanges(nsTArray<nsByteRange>& aRanges)
{
  return mCacheStream.GetCachedRanges(aRanges);
}

void nsMediaChannelStream::Suspend(bool aCloseImmediately)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  if (!element) {
    
    return;
  }

  if (mChannel) {
    if (aCloseImmediately && mCacheStream.IsSeekable()) {
      
      mIgnoreClose = true;
      CloseChannel();
      element->DownloadSuspended();
    } else if (mSuspendCount == 0) {
      {
        MutexAutoLock lock(mLock);
        mChannelStatistics.Stop(TimeStamp::Now());
      }
      PossiblySuspend();
      element->DownloadSuspended();
    }
  }

  ++mSuspendCount;
}

void nsMediaChannelStream::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
  NS_ASSERTION(mSuspendCount > 0, "Too many resumes!");

  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  if (!element) {
    
    return;
  }

  NS_ASSERTION(mSuspendCount > 0, "Resume without previous Suspend!");
  --mSuspendCount;
  if (mSuspendCount == 0) {
    if (mChannel) {
      
      {
        MutexAutoLock lock(mLock);
        mChannelStatistics.Start(TimeStamp::Now());
      }
      
      
      mReopenOnError = true;
      PossiblyResume();
      element->DownloadResumed();
    } else {
      PRInt64 totalLength = mCacheStream.GetLength();
      
      
      
      
      
      if (totalLength < 0 || mOffset < totalLength) {
        
        
        CacheClientSeek(mOffset, false);
      }
      element->DownloadResumed();
    }
  }
}

nsresult
nsMediaChannelStream::RecreateChannel()
{
  nsLoadFlags loadFlags =
    nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY |
    (mLoadInBackground ? nsIRequest::LOAD_BACKGROUND : 0);

  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  if (!element) {
    
    return NS_OK;
  }
  nsCOMPtr<nsILoadGroup> loadGroup = element->GetDocumentLoadGroup();
  NS_ENSURE_TRUE(loadGroup, NS_ERROR_NULL_POINTER);

  return NS_NewChannel(getter_AddRefs(mChannel),
                       mURI,
                       nsnull,
                       loadGroup,
                       nsnull,
                       loadFlags);
}

void
nsMediaChannelStream::DoNotifyDataReceived()
{
  mDataReceivedEvent.Revoke();
  mDecoder->NotifyBytesDownloaded();
}

void
nsMediaChannelStream::CacheClientNotifyDataReceived()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
  
  

  if (mDataReceivedEvent.IsPending())
    return;

  mDataReceivedEvent =
    NS_NewNonOwningRunnableMethod(this, &nsMediaChannelStream::DoNotifyDataReceived);
  NS_DispatchToMainThread(mDataReceivedEvent.get(), NS_DISPATCH_NORMAL);
}

class DataEnded : public nsRunnable {
public:
  DataEnded(nsMediaDecoder* aDecoder, nsresult aStatus) :
    mDecoder(aDecoder), mStatus(aStatus) {}
  NS_IMETHOD Run() {
    mDecoder->NotifyDownloadEnded(mStatus);
    return NS_OK;
  }
private:
  nsRefPtr<nsMediaDecoder> mDecoder;
  nsresult                 mStatus;
};

void
nsMediaChannelStream::CacheClientNotifyDataEnded(nsresult aStatus)
{
  printf("*** nsMediaChannelStream::CacheClientNotifyDataEnded() mDecoder=%p\n", mDecoder);

  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
  
  

  nsCOMPtr<nsIRunnable> event = new DataEnded(mDecoder, aStatus);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

nsresult
nsMediaChannelStream::CacheClientSeek(PRInt64 aOffset, bool aResume)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  printf("*** nsMediaChannelStream::CacheClientSeek() mDecoder=%p aOffset=%lld aResume = %d\n", mDecoder, (long long)aOffset, aResume);

  CloseChannel();

  if (aResume) {
    NS_ASSERTION(mSuspendCount > 0, "Too many resumes!");
    
    --mSuspendCount;
  }

  nsresult rv = RecreateChannel();
  if (NS_FAILED(rv))
    return rv;

  mOffset = aOffset;
  return OpenChannel(nsnull);
}

nsresult
nsMediaChannelStream::CacheClientSuspend()
{
  printf("*** nsMediaChannelStream::CacheClientSuspend() mDecoder=%p\n", mDecoder);

  Suspend(false);

  mDecoder->NotifySuspendedStatusChanged();
  return NS_OK;
}

nsresult
nsMediaChannelStream::CacheClientResume()
{
  printf("*** nsMediaChannelStream::CacheClientResume() mDecoder=%p\n", mDecoder);

  Resume();

  mDecoder->NotifySuspendedStatusChanged();
  return NS_OK;
}

PRInt64
nsMediaChannelStream::GetNextCachedData(PRInt64 aOffset)
{
  return mCacheStream.GetNextCachedData(aOffset);
}

PRInt64
nsMediaChannelStream::GetCachedDataEnd(PRInt64 aOffset)
{
  return mCacheStream.GetCachedDataEnd(aOffset);
}

bool
nsMediaChannelStream::IsDataCachedToEndOfStream(PRInt64 aOffset)
{
  return mCacheStream.IsDataCachedToEndOfStream(aOffset);
}

void
nsMediaChannelStream::EnsureCacheUpToDate()
{
  mCacheStream.EnsureCacheUpdate();
}

bool
nsMediaChannelStream::IsSuspendedByCache()
{
  return mCacheStream.AreAllStreamsForResourceSuspended();
}

bool
nsMediaChannelStream::IsSuspended()
{
  MutexAutoLock lock(mLock);
  return mSuspendCount > 0;
}

void
nsMediaChannelStream::SetReadMode(nsMediaCacheStream::ReadMode aMode)
{
  mCacheStream.SetReadMode(aMode);
}

void
nsMediaChannelStream::SetPlaybackRate(PRUint32 aBytesPerSecond)
{
  mCacheStream.SetPlaybackRate(aBytesPerSecond);
}

void
nsMediaChannelStream::Pin()
{
  mCacheStream.Pin();
}

void
nsMediaChannelStream::Unpin()
{
  mCacheStream.Unpin();
}

double
nsMediaChannelStream::GetDownloadRate(bool* aIsReliable)
{
  MutexAutoLock lock(mLock);
  return mChannelStatistics.GetRate(TimeStamp::Now(), aIsReliable);
}

PRInt64
nsMediaChannelStream::GetLength()
{
  return mCacheStream.GetLength();
}

void
nsMediaChannelStream::PossiblySuspend()
{
  bool isPending = false;
  nsresult rv = mChannel->IsPending(&isPending);
  if (NS_SUCCEEDED(rv) && isPending) {
    mChannel->Suspend();
    mIgnoreResume = false;
  } else {
    mIgnoreResume = true;
  }
}

void
nsMediaChannelStream::PossiblyResume()
{
  if (!mIgnoreResume) {
    mChannel->Resume();
  } else {
    mIgnoreResume = false;
  }
}

class nsMediaFileStream : public nsMediaStream
{
public:
  nsMediaFileStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsMediaStream(aDecoder, aChannel, aURI), mSize(-1),
    mLock("nsMediaFileStream.mLock")
  {
  }
  ~nsMediaFileStream()
  {
  }

  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual void     Suspend(bool aCloseImmediately) {}
  virtual void     Resume() {}
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();
  virtual nsMediaStream* CloneData(nsMediaDecoder* aDecoder);
  virtual nsresult ReadFromCache(char* aBuffer, PRInt64 aOffset, PRUint32 aCount);

  

  
  virtual void     SetReadMode(nsMediaCacheStream::ReadMode aMode) {}
  virtual void     SetPlaybackRate(PRUint32 aBytesPerSecond) {}
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();

  
  virtual void    Pin() {}
  virtual void    Unpin() {}
  virtual double  GetDownloadRate(bool* aIsReliable)
  {
    
    *aIsReliable = true;
    return 100*1024*1024; 
  }
  virtual PRInt64 GetLength() { return mSize; }
  virtual PRInt64 GetNextCachedData(PRInt64 aOffset)
  {
    return (aOffset < mSize) ? aOffset : -1;
  }
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset) { return NS_MAX(aOffset, mSize); }
  virtual bool    IsDataCachedToEndOfStream(PRInt64 aOffset) { return true; }
  virtual bool    IsSuspendedByCache() { return false; }
  virtual bool    IsSuspended() { return false; }

  nsresult GetCachedRanges(nsTArray<nsByteRange>& aRanges);

private:
  
  PRInt64 mSize;

  
  
  
  
  
  Mutex mLock;

  
  
  nsCOMPtr<nsISeekableStream> mSeekable;

  
  
  nsCOMPtr<nsIInputStream>  mInput;
};

class LoadedEvent : public nsRunnable
{
public:
  LoadedEvent(nsMediaDecoder* aDecoder) :
    mDecoder(aDecoder)
  {
    MOZ_COUNT_CTOR(LoadedEvent);
  }
  ~LoadedEvent()
  {
    MOZ_COUNT_DTOR(LoadedEvent);
  }

  NS_IMETHOD Run() {
    mDecoder->NotifyDownloadEnded(NS_OK);
    return NS_OK;
  }

private:
  nsRefPtr<nsMediaDecoder> mDecoder;
};

nsresult nsMediaFileStream::GetCachedRanges(nsTArray<nsByteRange>& aRanges)
{
  if (mSize == -1) {
    return NS_ERROR_FAILURE;
  }
  aRanges.AppendElement(nsByteRange(0, mSize));
  return NS_OK;
}

nsresult nsMediaFileStream::Open(nsIStreamListener** aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  nsresult rv = NS_OK;
  if (aStreamListener) {
    
    
    
    nsCOMPtr<nsIFileChannel> fc(do_QueryInterface(mChannel));
    if (!fc)
      return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIFile> file;
    rv = fc->GetFile(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewLocalFileInputStream(getter_AddRefs(mInput), file);
  } else {
    
    
    nsHTMLMediaElement* element = mDecoder->GetMediaElement();
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);

    rv = nsContentUtils::GetSecurityManager()->
           CheckLoadURIWithPrincipal(element->NodePrincipal(),
                                     mURI,
                                     nsIScriptSecurityManager::STANDARD);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mChannel->Open(getter_AddRefs(mInput));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mSeekable = do_QueryInterface(mInput);
  if (!mSeekable) {
    
    
    
    
    return NS_ERROR_FAILURE;
  }

  
  
  PRUint32 size;
  rv = mInput->Available(&size);
  if (NS_SUCCEEDED(rv)) {
    mSize = size;
  }

  nsCOMPtr<nsIRunnable> event = new LoadedEvent(mDecoder);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  return NS_OK;
}

nsresult nsMediaFileStream::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  MutexAutoLock lock(mLock);
  if (mChannel) {
    mChannel->Cancel(NS_ERROR_PARSED_DATA_CACHED);
    mChannel = nsnull;
    mInput = nsnull;
    mSeekable = nsnull;
  }

  return NS_OK;
}

already_AddRefed<nsIPrincipal> nsMediaFileStream::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIPrincipal> principal;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (!secMan || !mChannel)
    return nsnull;
  secMan->GetChannelPrincipal(mChannel, getter_AddRefs(principal));
  return principal.forget();
}

nsMediaStream* nsMediaFileStream::CloneData(nsMediaDecoder* aDecoder)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsHTMLMediaElement* element = aDecoder->GetMediaElement();
  if (!element) {
    
    return nsnull;
  }
  nsCOMPtr<nsILoadGroup> loadGroup = element->GetDocumentLoadGroup();
  NS_ENSURE_TRUE(loadGroup, nsnull);

  nsCOMPtr<nsIChannel> channel;
  nsresult rv =
    NS_NewChannel(getter_AddRefs(channel), mURI, nsnull, loadGroup, nsnull, 0);
  if (NS_FAILED(rv))
    return nsnull;

  return new nsMediaFileStream(aDecoder, channel, mURI);
}

nsresult nsMediaFileStream::ReadFromCache(char* aBuffer, PRInt64 aOffset, PRUint32 aCount)
{
  MutexAutoLock lock(mLock);
  if (!mInput || !mSeekable)
    return NS_ERROR_FAILURE;
  PRInt64 offset = 0;
  nsresult res = mSeekable->Tell(&offset);
  NS_ENSURE_SUCCESS(res,res);
  res = mSeekable->Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
  NS_ENSURE_SUCCESS(res,res);
  PRUint32 bytesRead = 0;
  do {
    PRUint32 x = 0;
    PRUint32 bytesToRead = aCount - bytesRead;
    res = mInput->Read(aBuffer, bytesToRead, &x);
    bytesRead += x;
  } while (bytesRead != aCount && res == NS_OK);

  
  
  nsresult seekres = mSeekable->Seek(nsISeekableStream::NS_SEEK_SET, offset);

  
  NS_ENSURE_SUCCESS(res,res);

  
  return seekres;
}

nsresult nsMediaFileStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  MutexAutoLock lock(mLock);
  if (!mInput)
    return NS_ERROR_FAILURE;
  return mInput->Read(aBuffer, aCount, aBytes);
}

nsresult nsMediaFileStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  MutexAutoLock lock(mLock);
  if (!mSeekable)
    return NS_ERROR_FAILURE;
  return mSeekable->Seek(aWhence, aOffset);
}

PRInt64 nsMediaFileStream::Tell()
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  MutexAutoLock lock(mLock);
  if (!mSeekable)
    return 0;

  PRInt64 offset = 0;
  mSeekable->Tell(&offset);
  return offset;
}

nsMediaStream*
nsMediaStream::Create(nsMediaDecoder* aDecoder, nsIChannel* aChannel)
{
  NS_ASSERTION(NS_IsMainThread(),
	             "nsMediaStream::Open called on non-main thread");

  
  
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsCOMPtr<nsIFileChannel> fc = do_QueryInterface(aChannel);
  if (fc) {
    return new nsMediaFileStream(aDecoder, aChannel, uri);
  }
  return new nsMediaChannelStream(aDecoder, aChannel, uri);
}

void nsMediaStream::MoveLoadsToBackground() {
  NS_ASSERTION(!mLoadInBackground, "Why are you calling this more than once?");
  mLoadInBackground = true;
  if (!mChannel) {
    
    return;
  }

  nsresult rv;
  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  if (!element) {
    NS_WARNING("Null element in nsMediaStream::MoveLoadsToBackground()");
    return;
  }

  bool isPending = false;
  if (NS_SUCCEEDED(mChannel->IsPending(&isPending)) &&
      isPending) {
    nsLoadFlags loadFlags;
    rv = mChannel->GetLoadFlags(&loadFlags);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadFlags() failed!");

    loadFlags |= nsIRequest::LOAD_BACKGROUND;
    ModifyLoadFlags(loadFlags);
  }
}

void nsMediaStream::ModifyLoadFlags(nsLoadFlags aFlags)
{
  nsCOMPtr<nsILoadGroup> loadGroup;
  nsresult rv = mChannel->GetLoadGroup(getter_AddRefs(loadGroup));
  NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadGroup() failed!");

  nsresult status;
  mChannel->GetStatus(&status);

  
  if (loadGroup &&
      NS_SUCCEEDED(status)) {
    rv = loadGroup->RemoveRequest(mChannel, nsnull, status);
    NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveRequest() failed!");
  }

  rv = mChannel->SetLoadFlags(aFlags);
  NS_ASSERTION(NS_SUCCEEDED(rv), "SetLoadFlags() failed!");

  if (loadGroup &&
      NS_SUCCEEDED(status)) {
    rv = loadGroup->AddRequest(mChannel, nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "AddRequest() failed!");
  }
}
