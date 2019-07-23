




































#include "nsDebug.h"
#include "nsMediaStream.h"
#include "nsMediaDecoder.h"
#include "nsNetUtil.h"
#include "nsAutoLock.h"
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

#define HTTP_OK_CODE 200
#define HTTP_PARTIAL_RESPONSE_CODE 206

using mozilla::TimeStamp;

nsMediaChannelStream::nsMediaChannelStream(nsMediaDecoder* aDecoder,
    nsIChannel* aChannel, nsIURI* aURI)
  : nsMediaStream(aDecoder, aChannel, aURI),
    mOffset(0), mSuspendCount(0), 
    mReopenOnError(PR_FALSE), mIgnoreClose(PR_FALSE),
    mCacheStream(this),
    mLock(nsAutoLock::NewLock("media.channel.stream")),
    mCacheSuspendCount(0)    
{
}

nsMediaChannelStream::~nsMediaChannelStream()
{
  if (mListener) {
    
    mListener->Revoke();
  }
  if (mLock) {
    nsAutoLock::DestroyLock(mLock);
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
nsMediaChannelStream::Listener::OnChannelRedirect(nsIChannel* aOldChannel,
                                                  nsIChannel* aNewChannel,
                                                  PRUint32 aFlags)
{
  if (!mStream)
    return NS_OK;
  return mStream->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
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
  if (element->ShouldCheckAllowOrigin()) {
    
    
    nsresult status;
    nsresult rv = aRequest->GetStatus(&status);
    if (NS_FAILED(rv) || status == NS_ERROR_DOM_BAD_URI) {
      mDecoder->NetworkError();
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(aRequest);
  PRBool seekable = PR_FALSE;
  if (hc) {
    nsCAutoString ranges;
    hc->GetResponseHeader(NS_LITERAL_CSTRING("Accept-Ranges"),
                          ranges);
    PRBool acceptsRanges = ranges.EqualsLiteral("bytes"); 

    if (mOffset == 0) {
      
      
      
      
      
      
      nsCAutoString durationText;
      PRInt32 ec = 0;
      nsresult rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-Content-Duration"), durationText);
      if (NS_FAILED(rv)) {
        rv = hc->GetResponseHeader(NS_LITERAL_CSTRING("X-AMZ-Meta-Content-Duration"), durationText);
      }

      if (NS_SUCCEEDED(rv)) {
        float duration = durationText.ToFloat(&ec);
        if (ec == NS_OK && duration >= 0) {
          mDecoder->SetDuration(PRInt64(NS_round(duration*1000)));
        }
      }
    }
 
    PRUint32 responseStatus = 0; 
    hc->GetResponseStatus(&responseStatus);
    if (mOffset > 0 && responseStatus == HTTP_OK_CODE) {
      
      
      
      mCacheStream.NotifyDataStarted(0);
      mOffset = 0;
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
  }
  mDecoder->SetSeekable(seekable);
  mCacheStream.SetSeekable(seekable);

  nsCOMPtr<nsICachingChannel> cc = do_QueryInterface(aRequest);
  if (cc) {
    PRBool fromCache = PR_FALSE;
    nsresult rv = cc->IsFromCache(&fromCache);
    if (NS_SUCCEEDED(rv) && !fromCache) {
      cc->SetCacheAsFile(PR_TRUE);
    }
  }

  {
    nsAutoLock lock(mLock);
    mChannelStatistics.Start(TimeStamp::Now());
  }

  mReopenOnError = PR_FALSE;
  mIgnoreClose = PR_FALSE;
  if (mSuspendCount > 0) {
    
    mChannel->Suspend();
  }

  
  
  mDecoder->Progress(PR_FALSE);

  return NS_OK;
}

nsresult
nsMediaChannelStream::OnStopRequest(nsIRequest* aRequest, nsresult aStatus)
{
  NS_ASSERTION(mChannel.get() == aRequest, "Wrong channel!");
  NS_ASSERTION(mSuspendCount == 0,
               "How can OnStopRequest fire while we're suspended?");

  {
    nsAutoLock lock(mLock);
    mChannelStatistics.Stop(TimeStamp::Now());
  }

  if (NS_FAILED(aStatus) && aStatus != NS_ERROR_PARSED_DATA_CACHED &&
      mReopenOnError) {
    nsresult rv = CacheClientSeek(mOffset, PR_FALSE);
    if (NS_SUCCEEDED(rv))
      return rv;
    
    
  }

  if (!mIgnoreClose) {
    mCacheStream.NotifyDataEnded(aStatus);
    if (mDecoder) {
      mDecoder->NotifyDownloadEnded(aStatus);
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
    nsAutoLock lock(mLock);
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
  mDecoder->NotifyBytesDownloaded();

  
  
  mDecoder->Progress(PR_FALSE);
  return NS_OK;
}

nsresult nsMediaChannelStream::Open(nsIStreamListener **aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (!mLock)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = mCacheStream.Init();
  if (NS_FAILED(rv))
    return rv;
  NS_ASSERTION(mOffset == 0, "Who set mOffset already?");
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
      listener = new nsCrossSiteListenerProxy(mListener,
                                              element->NodePrincipal(),
                                              mChannel,
                                              PR_FALSE,
                                              &rv);
      NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
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
    hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString, PR_FALSE);
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
    stream->RecreateChannel();
    
  }
  return stream;
}

void nsMediaChannelStream::CloseChannel()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  {
    nsAutoLock lock(mLock);
    mChannelStatistics.Stop(TimeStamp::Now());
  }

  if (mListener) {
    mListener->Revoke();
    mListener = nsnull;
  }

  if (mChannel) {
    if (mSuspendCount > 0) {
      
      mChannel->Resume();
    }
    
    
    
    
    
    
    
    mChannel->Cancel(NS_ERROR_PARSED_DATA_CACHED);
    mChannel = nsnull;
  }
}

nsresult nsMediaChannelStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
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

void nsMediaChannelStream::Suspend(PRBool aCloseImmediately)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on main thread");

  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  if (!element) {
    
    return;
  }

  if (mChannel) {
    if (aCloseImmediately && mCacheStream.IsSeekable()) {
      
      mIgnoreClose = PR_TRUE;
      CloseChannel();
      element->DownloadSuspended();
    } else if (mSuspendCount == 0) {
      {
        nsAutoLock lock(mLock);
        mChannelStatistics.Stop(TimeStamp::Now());
      }
      mChannel->Suspend();
      element->DownloadSuspended();
    }
  }

  ++mSuspendCount;
}

void nsMediaChannelStream::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on main thread");
  NS_ASSERTION(mSuspendCount > 0, "Too many resumes!");

  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  if (!element) {
    
    return;
  }

  --mSuspendCount;
  if (mSuspendCount == 0) {
    if (mChannel) {
      
      {
        nsAutoLock lock(mLock);
        mChannelStatistics.Start(TimeStamp::Now());
      }
      
      
      mReopenOnError = PR_TRUE;
      mChannel->Resume();
      element->DownloadResumed();
    } else {
      
      CacheClientSeek(mOffset, PR_FALSE);
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

nsresult
nsMediaChannelStream::CacheClientSeek(PRInt64 aOffset, PRBool aResume)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on main thread");

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

class SuspendedStatusChanged : public nsRunnable 
{
public:
  SuspendedStatusChanged(nsMediaDecoder* aDecoder) :
    mDecoder(aDecoder)
  {
    MOZ_COUNT_CTOR(SuspendedStatusChanged);
  }
  ~SuspendedStatusChanged()
  {
    MOZ_COUNT_DTOR(SuspendedStatusChanged);
  }

  NS_IMETHOD Run() {
    mDecoder->NotifySuspendedStatusChanged();
    return NS_OK;
  }

private:
  nsRefPtr<nsMediaDecoder> mDecoder;
};

nsresult
nsMediaChannelStream::CacheClientSuspend()
{
  {
    nsAutoLock lock(mLock);
    ++mCacheSuspendCount;
  }
  Suspend(PR_FALSE);

  
  
  
  nsCOMPtr<nsIRunnable> event = new SuspendedStatusChanged(mDecoder);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  return NS_OK;
}

nsresult
nsMediaChannelStream::CacheClientResume()
{
  Resume();
  {
    nsAutoLock lock(mLock);
    --mCacheSuspendCount;
  }

  
  
  
  nsCOMPtr<nsIRunnable> event = new SuspendedStatusChanged(mDecoder);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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

PRBool
nsMediaChannelStream::IsDataCachedToEndOfStream(PRInt64 aOffset)
{
  return mCacheStream.IsDataCachedToEndOfStream(aOffset);
}

PRBool
nsMediaChannelStream::IsSuspendedByCache()
{
  nsAutoLock lock(mLock);
  return mCacheSuspendCount > 0;
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
nsMediaChannelStream::GetDownloadRate(PRPackedBool* aIsReliable)
{
  nsAutoLock lock(mLock);
  return mChannelStatistics.GetRate(TimeStamp::Now(), aIsReliable);
}

PRInt64
nsMediaChannelStream::GetLength()
{
  return mCacheStream.GetLength();
}

class nsMediaFileStream : public nsMediaStream
{
public:
  nsMediaFileStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    nsMediaStream(aDecoder, aChannel, aURI), mSize(-1),
    mLock(nsAutoLock::NewLock("media.file.stream"))
  {
  }
  ~nsMediaFileStream()
  {
    if (mLock) {
      nsAutoLock::DestroyLock(mLock);
    }
  }

  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual void     Suspend(PRBool aCloseImmediately) {}
  virtual void     Resume() {}
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();
  virtual nsMediaStream* CloneData(nsMediaDecoder* aDecoder);

  

  
  virtual void     SetReadMode(nsMediaCacheStream::ReadMode aMode) {}
  virtual void     SetPlaybackRate(PRUint32 aBytesPerSecond) {}
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();

  
  virtual void    Pin() {}
  virtual void    Unpin() {}
  virtual double  GetDownloadRate(PRPackedBool* aIsReliable)
  {
    
    *aIsReliable = PR_TRUE;
    return 100*1024*1024; 
  }
  virtual PRInt64 GetLength() { return mSize; }
  virtual PRInt64 GetNextCachedData(PRInt64 aOffset)
  {
    return (aOffset < mSize) ? aOffset : -1;
  }
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset) { return PR_MAX(aOffset, mSize); }
  virtual PRBool  IsDataCachedToEndOfStream(PRInt64 aOffset) { return PR_TRUE; }
  virtual PRBool  IsSuspendedByCache() { return PR_FALSE; }

private:
  
  PRInt64 mSize;

  
  
  
  
  
  PRLock* mLock;

  
  
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

nsresult nsMediaFileStream::Open(nsIStreamListener** aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  nsresult rv;
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

  nsAutoLock lock(mLock);
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

nsresult nsMediaFileStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  nsAutoLock lock(mLock);
  if (!mInput)
    return NS_ERROR_FAILURE;
  return mInput->Read(aBuffer, aCount, aBytes);
}

nsresult nsMediaFileStream::Seek(PRInt32 aWhence, PRInt64 aOffset) 
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  nsAutoLock lock(mLock);
  if (!mSeekable)
    return NS_ERROR_FAILURE;
  return mSeekable->Seek(aWhence, aOffset);
}

PRInt64 nsMediaFileStream::Tell()
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  nsAutoLock lock(mLock);
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
  mLoadInBackground = PR_TRUE;
  if (!mChannel) {
    
    return;
  }

  nsresult rv;
  nsHTMLMediaElement* element = mDecoder->GetMediaElement();
  if (!element) {
    NS_WARNING("Null element in nsMediaStream::MoveLoadsToBackground()");
    return;
  }
  nsCOMPtr<nsILoadGroup> loadGroup;
  rv = mChannel->GetLoadGroup(getter_AddRefs(loadGroup));
  NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadGroup() failed!");
  nsresult status;
  mChannel->GetStatus(&status);
  
  PRBool isPending = PR_FALSE;
  if (loadGroup &&
      NS_SUCCEEDED(status) &&
      NS_SUCCEEDED(mChannel->IsPending(&isPending)) &&
      isPending) {
    rv = loadGroup->RemoveRequest(mChannel, nsnull, status);
    NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveRequest() failed!");

    nsLoadFlags loadFlags;
    rv = mChannel->GetLoadFlags(&loadFlags);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadFlags() failed!");

    loadFlags |= nsIRequest::LOAD_BACKGROUND;
    rv = mChannel->SetLoadFlags(loadFlags);
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetLoadFlags() failed!");

    rv = loadGroup->AddRequest(mChannel, nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "AddRequest() failed!");
  }
}
