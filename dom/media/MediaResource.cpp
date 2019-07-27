





#include "mozilla/DebugOnly.h"

#include "MediaResource.h"
#include "RtspMediaResource.h"

#include "mozilla/Mutex.h"
#include "nsDebug.h"
#include "MediaDecoder.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsIFile.h"
#include "nsIFileChannel.h"
#include "nsIHttpChannel.h"
#include "nsISeekableStream.h"
#include "nsIInputStream.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsIScriptSecurityManager.h"
#include "nsCORSListenerProxy.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "nsError.h"
#include "nsICachingChannel.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsContentUtils.h"
#include "nsHostObjectProtocolHandler.h"
#include <algorithm>
#include "nsProxyRelease.h"
#include "nsIContentPolicy.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaResourceLog;
#define RESOURCE_LOG(msg, ...) PR_LOG(gMediaResourceLog, PR_LOG_DEBUG, \
                                      (msg, ##__VA_ARGS__))

#define CMLOG(msg, ...) \
        RESOURCE_LOG("%p [ChannelMediaResource]: " msg, this, ##__VA_ARGS__)
#else
#define RESOURCE_LOG(msg, ...)
#define CMLOG(msg, ...)
#endif

static const uint32_t HTTP_OK_CODE = 200;
static const uint32_t HTTP_PARTIAL_RESPONSE_CODE = 206;

namespace mozilla {

void
MediaResource::Destroy()
{
  
  
  
  
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    NS_ENSURE_TRUE_VOID(mainThread);
    nsRefPtr<MediaResource> doomed(this);
    if (NS_FAILED(NS_ProxyRelease(mainThread, doomed, true))) {
      NS_WARNING("Failed to proxy release to main thread!");
    }
  } else {
    delete this;
  }
}

NS_IMPL_ADDREF(MediaResource)
NS_IMPL_RELEASE_WITH_DESTROY(MediaResource, Destroy())
NS_IMPL_QUERY_INTERFACE0(MediaResource)

ChannelMediaResource::ChannelMediaResource(MediaDecoder* aDecoder,
                                           nsIChannel* aChannel,
                                           nsIURI* aURI,
                                           const nsACString& aContentType)
  : BaseMediaResource(aDecoder, aChannel, aURI, aContentType),
    mOffset(0), mSuspendCount(0),
    mReopenOnError(false), mIgnoreClose(false),
    mCacheStream(this),
    mLock("ChannelMediaResource.mLock"),
    mIgnoreResume(false),
    mIsTransportSeekable(true)
{
#ifdef PR_LOGGING
  if (!gMediaResourceLog) {
    gMediaResourceLog = PR_NewLogModule("MediaResource");
  }
#endif
}

ChannelMediaResource::~ChannelMediaResource()
{
  if (mListener) {
    
    mListener->Revoke();
  }
}







NS_IMPL_ISUPPORTS(ChannelMediaResource::Listener,
                  nsIRequestObserver, nsIStreamListener, nsIChannelEventSink,
                  nsIInterfaceRequestor)

nsresult
ChannelMediaResource::Listener::OnStartRequest(nsIRequest* aRequest,
                                               nsISupports* aContext)
{
  if (!mResource)
    return NS_OK;
  return mResource->OnStartRequest(aRequest);
}

nsresult
ChannelMediaResource::Listener::OnStopRequest(nsIRequest* aRequest,
                                              nsISupports* aContext,
                                              nsresult aStatus)
{
  if (!mResource)
    return NS_OK;
  return mResource->OnStopRequest(aRequest, aStatus);
}

nsresult
ChannelMediaResource::Listener::OnDataAvailable(nsIRequest* aRequest,
                                                nsISupports* aContext,
                                                nsIInputStream* aStream,
                                                uint64_t aOffset,
                                                uint32_t aCount)
{
  if (!mResource)
    return NS_OK;
  return mResource->OnDataAvailable(aRequest, aStream, aCount);
}

nsresult
ChannelMediaResource::Listener::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                                       nsIChannel* aNewChannel,
                                                       uint32_t aFlags,
                                                       nsIAsyncVerifyRedirectCallback* cb)
{
  nsresult rv = NS_OK;
  if (mResource)
    rv = mResource->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);

  if (NS_FAILED(rv))
    return rv;

  cb->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

nsresult
ChannelMediaResource::Listener::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}

nsresult
ChannelMediaResource::OnStartRequest(nsIRequest* aRequest)
{
  NS_ASSERTION(mChannel.get() == aRequest, "Wrong channel!");

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if (status == NS_BINDING_ABORTED) {
    
    
    
    
    
    CloseChannel();
    return status;
  }

  if (element->ShouldCheckAllowOrigin()) {
    
    
    if (status == NS_ERROR_DOM_BAD_URI) {
      mDecoder->NetworkError();
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(aRequest);
  bool seekable = false;
  if (hc) {
    uint32_t responseStatus = 0;
    hc->GetResponseStatus(&responseStatus);
    bool succeeded = false;
    hc->GetRequestSucceeded(&succeeded);

    if (!succeeded && NS_SUCCEEDED(status)) {
      
      
      
      
      
      
      
      
      
      if (responseStatus == HTTP_REQUESTED_RANGE_NOT_SATISFIABLE_CODE) {
        
        
        mCacheStream.NotifyDataEnded(status);
      } else {
        mDecoder->NetworkError();
      }

      
      
      CloseChannel();
      return NS_OK;
    }

    nsAutoCString ranges;
    hc->GetResponseHeader(NS_LITERAL_CSTRING("Accept-Ranges"),
                          ranges);
    bool acceptsRanges = ranges.EqualsLiteral("bytes");
    
    bool dataIsBounded = false;

    int64_t contentLength = -1;
    hc->GetContentLength(&contentLength);
    if (contentLength >= 0 && responseStatus == HTTP_OK_CODE) {
      
      
      dataIsBounded = true;
    }

    if (mOffset == 0) {
      
      
      
      
      
      
      
      
      nsAutoCString durationText;
      nsresult ec = NS_OK;
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
          
          dataIsBounded = true;
        }
      }
    }

    
    
    bool boundedSeekLimit = true;
    
    if (!mByteRange.IsNull() && (responseStatus == HTTP_PARTIAL_RESPONSE_CODE)) {
      
      int64_t rangeStart = 0;
      int64_t rangeEnd = 0;
      int64_t rangeTotal = 0;
      rv = ParseContentRangeHeader(hc, rangeStart, rangeEnd, rangeTotal);
      if (NS_FAILED(rv)) {
        
        CMLOG("Error processing \'Content-Range' for "
              "HTTP_PARTIAL_RESPONSE_CODE: rv[%x] channel[%p] decoder[%p]",
              rv, hc.get(), mDecoder);
        mDecoder->NetworkError();
        CloseChannel();
        return NS_OK;
      }

      
      
      NS_WARN_IF_FALSE(mByteRange.mStart == rangeStart,
                       "response range start does not match request");
      NS_WARN_IF_FALSE(mOffset == rangeStart,
                       "response range start does not match current offset");
      NS_WARN_IF_FALSE(mByteRange.mEnd == rangeEnd,
                       "response range end does not match request");
      
      
      
      if (rangeTotal == -1) {
        boundedSeekLimit = false;
      } else {
        mCacheStream.NotifyDataLength(rangeTotal);
      }
      mCacheStream.NotifyDataStarted(rangeStart);

      mOffset = rangeStart;
      
      acceptsRanges = true;
    } else if (((mOffset > 0) || !mByteRange.IsNull())
               && (responseStatus == HTTP_OK_CODE)) {
      
      
      
      mCacheStream.NotifyDataStarted(0);
      mOffset = 0;

      
      acceptsRanges = false;
    } else if (mOffset == 0 &&
               (responseStatus == HTTP_OK_CODE ||
                responseStatus == HTTP_PARTIAL_RESPONSE_CODE)) {
      if (contentLength >= 0) {
        mCacheStream.NotifyDataLength(contentLength);
      }
    }
    
    

    
    
    
    seekable =
      responseStatus == HTTP_PARTIAL_RESPONSE_CODE || acceptsRanges;
    if (seekable && boundedSeekLimit) {
      
      
      dataIsBounded = true;
    }

    mDecoder->SetInfinite(!dataIsBounded);
  }
  mCacheStream.SetTransportSeekable(seekable);

  {
    MutexAutoLock lock(mLock);
    mIsTransportSeekable = seekable;
    mChannelStatistics->Start();
  }

  mReopenOnError = false;
  mIgnoreClose = false;

  if (mSuspendCount > 0) {
    
    
    
    mChannel->Suspend();
    mIgnoreResume = false;
  }

  
  owner->DownloadProgressed();

  return NS_OK;
}

bool
ChannelMediaResource::IsTransportSeekable()
{
  MutexAutoLock lock(mLock);
  return mIsTransportSeekable;
}

nsresult
ChannelMediaResource::ParseContentRangeHeader(nsIHttpChannel * aHttpChan,
                                              int64_t& aRangeStart,
                                              int64_t& aRangeEnd,
                                              int64_t& aRangeTotal)
{
  NS_ENSURE_ARG(aHttpChan);

  nsAutoCString rangeStr;
  nsresult rv = aHttpChan->GetResponseHeader(NS_LITERAL_CSTRING("Content-Range"),
                                             rangeStr);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_FALSE(rangeStr.IsEmpty(), NS_ERROR_ILLEGAL_VALUE);

  
  int32_t spacePos = rangeStr.Find(NS_LITERAL_CSTRING(" "));
  int32_t dashPos = rangeStr.Find(NS_LITERAL_CSTRING("-"), true, spacePos);
  int32_t slashPos = rangeStr.Find(NS_LITERAL_CSTRING("/"), true, dashPos);

  nsAutoCString aRangeStartText;
  rangeStr.Mid(aRangeStartText, spacePos+1, dashPos-(spacePos+1));
  aRangeStart = aRangeStartText.ToInteger64(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(0 <= aRangeStart, NS_ERROR_ILLEGAL_VALUE);

  nsAutoCString aRangeEndText;
  rangeStr.Mid(aRangeEndText, dashPos+1, slashPos-(dashPos+1));
  aRangeEnd = aRangeEndText.ToInteger64(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aRangeStart < aRangeEnd, NS_ERROR_ILLEGAL_VALUE);

  nsAutoCString aRangeTotalText;
  rangeStr.Right(aRangeTotalText, rangeStr.Length()-(slashPos+1));
  if (aRangeTotalText[0] == '*') {
    aRangeTotal = -1;
  } else {
    aRangeTotal = aRangeTotalText.ToInteger64(&rv);
    NS_ENSURE_TRUE(aRangeEnd < aRangeTotal, NS_ERROR_ILLEGAL_VALUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  CMLOG("Received bytes [%lld] to [%lld] of [%lld] for decoder[%p]",
        aRangeStart, aRangeEnd, aRangeTotal, mDecoder);

  return NS_OK;
}

nsresult
ChannelMediaResource::OnStopRequest(nsIRequest* aRequest, nsresult aStatus)
{
  NS_ASSERTION(mChannel.get() == aRequest, "Wrong channel!");
  NS_ASSERTION(mSuspendCount == 0,
               "How can OnStopRequest fire while we're suspended?");

  {
    MutexAutoLock lock(mLock);
    mChannelStatistics->Stop();
  }

  
  
  
  
  
  
  if (mReopenOnError &&
      aStatus != NS_ERROR_PARSED_DATA_CACHED && aStatus != NS_BINDING_ABORTED &&
      (mOffset == 0 || mCacheStream.IsTransportSeekable())) {
    
    
    
    nsresult rv = CacheClientSeek(mOffset, false);
    if (NS_SUCCEEDED(rv))
      return rv;
    
    
  }

  if (!mIgnoreClose) {
    mCacheStream.NotifyDataEnded(aStatus);

    
    
    
    nsLoadFlags loadFlags;
    DebugOnly<nsresult> rv = mChannel->GetLoadFlags(&loadFlags);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadFlags() failed!");

    if (loadFlags & nsIRequest::LOAD_BACKGROUND) {
      ModifyLoadFlags(loadFlags & ~nsIRequest::LOAD_BACKGROUND);
    }
  }

  return NS_OK;
}

nsresult
ChannelMediaResource::OnChannelRedirect(nsIChannel* aOld, nsIChannel* aNew,
                                        uint32_t aFlags)
{
  mChannel = aNew;
  return SetupChannelHeaders();
}

struct CopySegmentClosure {
  nsCOMPtr<nsIPrincipal> mPrincipal;
  ChannelMediaResource*  mResource;
};

NS_METHOD
ChannelMediaResource::CopySegmentToCache(nsIInputStream *aInStream,
                                         void *aClosure,
                                         const char *aFromSegment,
                                         uint32_t aToOffset,
                                         uint32_t aCount,
                                         uint32_t *aWriteCount)
{
  CopySegmentClosure* closure = static_cast<CopySegmentClosure*>(aClosure);

  closure->mResource->mDecoder->NotifyDataArrived(aFromSegment, aCount, closure->mResource->mOffset);

  
  RESOURCE_LOG("%p [ChannelMediaResource]: CopySegmentToCache at mOffset [%lld] add "
               "[%d] bytes for decoder[%p]",
               closure->mResource, closure->mResource->mOffset, aCount,
               closure->mResource->mDecoder);
  closure->mResource->mOffset += aCount;

  closure->mResource->mCacheStream.NotifyDataReceived(aCount, aFromSegment,
                                                      closure->mPrincipal);
  *aWriteCount = aCount;
  return NS_OK;
}

nsresult
ChannelMediaResource::OnDataAvailable(nsIRequest* aRequest,
                                      nsIInputStream* aStream,
                                      uint32_t aCount)
{
  NS_ASSERTION(mChannel.get() == aRequest, "Wrong channel!");

  {
    MutexAutoLock lock(mLock);
    mChannelStatistics->AddBytes(aCount);
  }

  CopySegmentClosure closure;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (secMan && mChannel) {
    secMan->GetChannelResultPrincipal(mChannel, getter_AddRefs(closure.mPrincipal));
  }
  closure.mResource = this;

  uint32_t count = aCount;
  while (count > 0) {
    uint32_t read;
    nsresult rv = aStream->ReadSegments(CopySegmentToCache, &closure, count,
                                        &read);
    if (NS_FAILED(rv))
      return rv;
    NS_ASSERTION(read > 0, "Read 0 bytes while data was available?");
    count -= read;
  }

  return NS_OK;
}

nsresult ChannelMediaResource::Open(nsIStreamListener **aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (!mChannelStatistics) {
    mChannelStatistics = new MediaChannelStatistics();
  }

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

nsresult ChannelMediaResource::OpenChannel(nsIStreamListener** aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ENSURE_TRUE(mChannel, NS_ERROR_NULL_POINTER);
  NS_ASSERTION(!mListener, "Listener should have been removed by now");

  if (aStreamListener) {
    *aStreamListener = nullptr;
  }

  if (mByteRange.IsNull()) {
    
    
    
    
    nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(mChannel);
    if (hc) {
      int64_t cl = -1;
      if (NS_SUCCEEDED(hc->GetContentLength(&cl)) && cl != -1) {
        mCacheStream.NotifyDataLength(cl);
      }
    }
  }

  mListener = new Listener(this);
  NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);

  if (aStreamListener) {
    *aStreamListener = mListener;
    NS_ADDREF(*aStreamListener);
  } else {
    nsresult rv = mChannel->SetNotificationCallbacks(mListener.get());
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStreamListener> listener = mListener.get();

    
    
    MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
    NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);
    dom::HTMLMediaElement* element = owner->GetMediaElement();
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
    if (element->ShouldCheckAllowOrigin()) {
      nsRefPtr<nsCORSListenerProxy> crossSiteListener =
        new nsCORSListenerProxy(mListener,
                                element->NodePrincipal(),
                                false);
      NS_ENSURE_TRUE(crossSiteListener, NS_ERROR_OUT_OF_MEMORY);
      rv = crossSiteListener->Init(mChannel, DataURIHandling::Allow);
      NS_ENSURE_SUCCESS(rv, rv);
      listener = crossSiteListener;
    } else {
      rv = nsContentUtils::GetSecurityManager()->
        CheckLoadURIWithPrincipal(element->NodePrincipal(),
                                  mURI,
                                  nsIScriptSecurityManager::STANDARD);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = SetupChannelHeaders();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mChannel->AsyncOpen(listener, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    element->DownloadResumed(true);
  }

  return NS_OK;
}

nsresult ChannelMediaResource::SetupChannelHeaders()
{
  
  
  
  
  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(mChannel);
  if (hc) {
    
    
    nsAutoCString rangeString("bytes=");
    if (!mByteRange.IsNull()) {
      rangeString.AppendInt(mByteRange.mStart);
      mOffset = mByteRange.mStart;
    } else {
      rangeString.AppendInt(mOffset);
    }
    rangeString.Append('-');
    if (!mByteRange.IsNull()) {
      rangeString.AppendInt(mByteRange.mEnd);
    }
    nsresult rv = hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString, false);
    NS_ENSURE_SUCCESS(rv, rv);

    
    NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
    MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
    NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);
    dom::HTMLMediaElement* element = owner->GetMediaElement();
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
    element->SetRequestHeaders(hc);
  } else {
    NS_ASSERTION(mOffset == 0, "Don't know how to seek on this channel type");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult ChannelMediaResource::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  mCacheStream.Close();
  CloseChannel();
  return NS_OK;
}

already_AddRefed<nsIPrincipal> ChannelMediaResource::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIPrincipal> principal = mCacheStream.GetCurrentPrincipal();
  return principal.forget();
}

bool ChannelMediaResource::CanClone()
{
  return mCacheStream.IsAvailableForSharing();
}

already_AddRefed<MediaResource> ChannelMediaResource::CloneData(MediaDecoder* aDecoder)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ASSERTION(mCacheStream.IsAvailableForSharing(), "Stream can't be cloned");

  nsRefPtr<ChannelMediaResource> resource =
    new ChannelMediaResource(aDecoder,
                             nullptr,
                             mURI,
                             GetContentType());
  if (resource) {
    
    
    
    
    
    
    resource->mSuspendCount = 1;
    resource->mCacheStream.InitAsClone(&mCacheStream);
    resource->mChannelStatistics = new MediaChannelStatistics(mChannelStatistics);
    resource->mChannelStatistics->Stop();
  }
  return resource.forget();
}

void ChannelMediaResource::CloseChannel()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  {
    MutexAutoLock lock(mLock);
    mChannelStatistics->Stop();
  }

  if (mListener) {
    mListener->Revoke();
    mListener = nullptr;
  }

  if (mChannel) {
    if (mSuspendCount > 0) {
      
      PossiblyResume();
    }
    
    
    
    
    
    
    
    mChannel->Cancel(NS_ERROR_PARSED_DATA_CACHED);
    mChannel = nullptr;
  }
}

nsresult ChannelMediaResource::ReadFromCache(char* aBuffer,
                                             int64_t aOffset,
                                             uint32_t aCount)
{
  return mCacheStream.ReadFromCache(aBuffer, aOffset, aCount);
}

nsresult ChannelMediaResource::Read(char* aBuffer,
                                    uint32_t aCount,
                                    uint32_t* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  int64_t offset = mCacheStream.Tell();
  nsresult rv = mCacheStream.Read(aBuffer, aCount, aBytes);
  if (NS_SUCCEEDED(rv)) {
    DispatchBytesConsumed(*aBytes, offset);
  }
  return rv;
}

nsresult ChannelMediaResource::ReadAt(int64_t aOffset,
                                      char* aBuffer,
                                      uint32_t aCount,
                                      uint32_t* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  nsresult rv = mCacheStream.ReadAt(aOffset, aBuffer, aCount, aBytes);
  if (NS_SUCCEEDED(rv)) {
    DispatchBytesConsumed(*aBytes, aOffset);
  }
  return rv;
}

nsresult ChannelMediaResource::Seek(int32_t aWhence, int64_t aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  CMLOG("Seek requested for aOffset [%lld] for decoder [%p]",
        aOffset, mDecoder);
  return mCacheStream.Seek(aWhence, aOffset);
}

int64_t ChannelMediaResource::Tell()
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  return mCacheStream.Tell();
}

nsresult ChannelMediaResource::GetCachedRanges(nsTArray<MediaByteRange>& aRanges)
{
  return mCacheStream.GetCachedRanges(aRanges);
}

void ChannelMediaResource::Suspend(bool aCloseImmediately)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  if (!owner) {
    
    return;
  }
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  if (!element) {
    
    return;
  }

  if (mChannel) {
    if (aCloseImmediately && mCacheStream.IsTransportSeekable()) {
      
      mIgnoreClose = true;
      CloseChannel();
      element->DownloadSuspended();
    } else if (mSuspendCount == 0) {
      {
        MutexAutoLock lock(mLock);
        mChannelStatistics->Stop();
      }
      PossiblySuspend();
      element->DownloadSuspended();
    }
  }

  ++mSuspendCount;
}

void ChannelMediaResource::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
  NS_ASSERTION(mSuspendCount > 0, "Too many resumes!");

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  if (!owner) {
    
    return;
  }
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  if (!element) {
    
    return;
  }

  NS_ASSERTION(mSuspendCount > 0, "Resume without previous Suspend!");
  --mSuspendCount;
  if (mSuspendCount == 0) {
    if (mChannel) {
      
      {
        MutexAutoLock lock(mLock);
        mChannelStatistics->Start();
      }
      
      
      mReopenOnError = true;
      PossiblyResume();
      element->DownloadResumed();
    } else {
      int64_t totalLength = mCacheStream.GetLength();
      
      
      
      
      
      if (totalLength < 0 || mOffset < totalLength) {
        
        
        CacheClientSeek(mOffset, false);
        element->DownloadResumed();
      } else {
        
        
      }
    }
  }
}

nsresult
ChannelMediaResource::RecreateChannel()
{
  nsLoadFlags loadFlags =
    nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY |
    (mLoadInBackground ? nsIRequest::LOAD_BACKGROUND : 0);

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  if (!owner) {
    
    return NS_OK;
  }
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  if (!element) {
    
    return NS_OK;
  }
  nsCOMPtr<nsILoadGroup> loadGroup = element->GetDocumentLoadGroup();
  NS_ENSURE_TRUE(loadGroup, NS_ERROR_NULL_POINTER);

  nsSecurityFlags securityFlags = nsILoadInfo::SEC_NORMAL;
  if (nsContentUtils::ChannelShouldInheritPrincipal(element->NodePrincipal(),
                                                    mURI,
                                                    false, 
                                                    false 
                                                    )) {
    securityFlags = nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL;
  }

  nsresult rv = NS_NewChannel(getter_AddRefs(mChannel),
                              mURI,
                              element,
                              securityFlags,
                              nsIContentPolicy::TYPE_MEDIA,
                              loadGroup,
                              nullptr,  
                              loadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  NS_ASSERTION(!GetContentType().IsEmpty(),
      "When recreating a channel, we should know the Content-Type.");
  mChannel->SetContentType(GetContentType());

  
  mCacheStream.NotifyChannelRecreated();

  return rv;
}

void
ChannelMediaResource::DoNotifyDataReceived()
{
  mDataReceivedEvent.Revoke();
  mDecoder->NotifyBytesDownloaded();
}

void
ChannelMediaResource::CacheClientNotifyDataReceived()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
  
  

  if (mDataReceivedEvent.IsPending())
    return;

  mDataReceivedEvent =
    NS_NewNonOwningRunnableMethod(this, &ChannelMediaResource::DoNotifyDataReceived);
  NS_DispatchToMainThread(mDataReceivedEvent.get());
}

class DataEnded : public nsRunnable {
public:
  DataEnded(MediaDecoder* aDecoder, nsresult aStatus) :
    mDecoder(aDecoder), mStatus(aStatus) {}
  NS_IMETHOD Run() {
    mDecoder->NotifyDownloadEnded(mStatus);
    if (NS_SUCCEEDED(mStatus)) {
      MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
      if (owner) {
        dom::HTMLMediaElement* element = owner->GetMediaElement();
        if (element) {
          element->DownloadSuspended();
        }
      }
      
      
      
      mDecoder->NotifySuspendedStatusChanged();
    }
    return NS_OK;
  }
private:
  nsRefPtr<MediaDecoder> mDecoder;
  nsresult               mStatus;
};

void
ChannelMediaResource::CacheClientNotifyDataEnded(nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  

  nsCOMPtr<nsIRunnable> event = new DataEnded(mDecoder, aStatus);
  NS_DispatchToCurrentThread(event);
}

void
ChannelMediaResource::CacheClientNotifyPrincipalChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  mDecoder->NotifyPrincipalChanged();
}

void
ChannelMediaResource::CacheClientNotifySuspendedStatusChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
  mDecoder->NotifySuspendedStatusChanged();
}

nsresult
ChannelMediaResource::CacheClientSeek(int64_t aOffset, bool aResume)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  CMLOG("CacheClientSeek requested for aOffset [%lld] for decoder [%p]",
        aOffset, mDecoder);

  CloseChannel();

  if (aResume) {
    NS_ASSERTION(mSuspendCount > 0, "Too many resumes!");
    
    --mSuspendCount;
  }

  mOffset = aOffset;

  if (mSuspendCount > 0) {
    
    
    if (mChannel) {
      mIgnoreClose = true;
      CloseChannel();
    }
    return NS_OK;
  }

  nsresult rv = RecreateChannel();
  NS_ENSURE_SUCCESS(rv, rv);

  return OpenChannel(nullptr);
}

void
ChannelMediaResource::FlushCache()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  mCacheStream.FlushPartialBlock();
}

void
ChannelMediaResource::NotifyLastByteRange()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  
  mCacheStream.NotifyDataEnded(NS_OK);

}

nsresult
ChannelMediaResource::CacheClientSuspend()
{
  Suspend(false);
  return NS_OK;
}

nsresult
ChannelMediaResource::CacheClientResume()
{
  Resume();
  return NS_OK;
}

int64_t
ChannelMediaResource::GetNextCachedData(int64_t aOffset)
{
  return mCacheStream.GetNextCachedData(aOffset);
}

int64_t
ChannelMediaResource::GetCachedDataEnd(int64_t aOffset)
{
  return mCacheStream.GetCachedDataEnd(aOffset);
}

bool
ChannelMediaResource::IsDataCachedToEndOfResource(int64_t aOffset)
{
  return mCacheStream.IsDataCachedToEndOfStream(aOffset);
}

void
ChannelMediaResource::EnsureCacheUpToDate()
{
  mCacheStream.EnsureCacheUpdate();
}

bool
ChannelMediaResource::IsSuspendedByCache()
{
  return mCacheStream.AreAllStreamsForResourceSuspended();
}

bool
ChannelMediaResource::IsSuspended()
{
  MutexAutoLock lock(mLock);
  return mSuspendCount > 0;
}

void
ChannelMediaResource::SetReadMode(MediaCacheStream::ReadMode aMode)
{
  mCacheStream.SetReadMode(aMode);
}

void
ChannelMediaResource::SetPlaybackRate(uint32_t aBytesPerSecond)
{
  mCacheStream.SetPlaybackRate(aBytesPerSecond);
}

void
ChannelMediaResource::Pin()
{
  mCacheStream.Pin();
}

void
ChannelMediaResource::Unpin()
{
  mCacheStream.Unpin();
}

double
ChannelMediaResource::GetDownloadRate(bool* aIsReliable)
{
  MutexAutoLock lock(mLock);
  return mChannelStatistics->GetRate(aIsReliable);
}

int64_t
ChannelMediaResource::GetLength()
{
  return mCacheStream.GetLength();
}

void
ChannelMediaResource::PossiblySuspend()
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
ChannelMediaResource::PossiblyResume()
{
  if (!mIgnoreResume) {
    mChannel->Resume();
  } else {
    mIgnoreResume = false;
  }
}

class FileMediaResource : public BaseMediaResource
{
public:
  FileMediaResource(MediaDecoder* aDecoder,
                    nsIChannel* aChannel,
                    nsIURI* aURI,
                    const nsACString& aContentType) :
    BaseMediaResource(aDecoder, aChannel, aURI, aContentType),
    mSize(-1),
    mLock("FileMediaResource.mLock"),
    mSizeInitialized(false)
  {
  }
  ~FileMediaResource()
  {
  }

  
  virtual nsresult Open(nsIStreamListener** aStreamListener) override;
  virtual nsresult Close() override;
  virtual void     Suspend(bool aCloseImmediately) override {}
  virtual void     Resume() override {}
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() override;
  virtual bool     CanClone() override;
  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder) override;
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount) override;

  

  
  virtual void     SetReadMode(MediaCacheStream::ReadMode aMode) override {}
  virtual void     SetPlaybackRate(uint32_t aBytesPerSecond) override {}
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes) override;
  virtual nsresult ReadAt(int64_t aOffset, char* aBuffer,
                          uint32_t aCount, uint32_t* aBytes) override;
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset) override;
  virtual int64_t  Tell() override;

  
  virtual void    Pin() override {}
  virtual void    Unpin() override {}
  virtual double  GetDownloadRate(bool* aIsReliable) override
  {
    
    *aIsReliable = true;
    return 100*1024*1024; 
  }
  virtual int64_t GetLength() override {
    MutexAutoLock lock(mLock);

    EnsureSizeInitialized();
    return mSizeInitialized ? mSize : 0;
  }
  virtual int64_t GetNextCachedData(int64_t aOffset) override
  {
    MutexAutoLock lock(mLock);

    EnsureSizeInitialized();
    return (aOffset < mSize) ? aOffset : -1;
  }
  virtual int64_t GetCachedDataEnd(int64_t aOffset) override {
    MutexAutoLock lock(mLock);

    EnsureSizeInitialized();
    return std::max(aOffset, mSize);
  }
  virtual bool    IsDataCachedToEndOfResource(int64_t aOffset) override { return true; }
  virtual bool    IsSuspendedByCache() override { return true; }
  virtual bool    IsSuspended() override { return true; }
  virtual bool    IsTransportSeekable() override { return true; }

  nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) override;

  virtual size_t SizeOfExcludingThis(
                        MallocSizeOf aMallocSizeOf) const override
  {
    
    
    return BaseMediaResource::SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual size_t SizeOfIncludingThis(
                        MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  
  
  
  
  nsresult UnsafeRead(char* aBuffer, uint32_t aCount, uint32_t* aBytes);
  nsresult UnsafeSeek(int32_t aWhence, int64_t aOffset);
private:
  
  
  void EnsureSizeInitialized();

  
  
  int64_t mSize;

  
  
  
  
  
  Mutex mLock;

  
  
  nsCOMPtr<nsISeekableStream> mSeekable;

  
  
  nsCOMPtr<nsIInputStream>  mInput;

  
  
  
  bool mSizeInitialized;
};

void FileMediaResource::EnsureSizeInitialized()
{
  mLock.AssertCurrentThreadOwns();
  NS_ASSERTION(mInput, "Must have file input stream");
  if (mSizeInitialized) {
    return;
  }
  mSizeInitialized = true;
  
  uint64_t size;
  nsresult res = mInput->Available(&size);
  if (NS_SUCCEEDED(res) && size <= INT64_MAX) {
    mSize = (int64_t)size;
    nsCOMPtr<nsIRunnable> event = new DataEnded(mDecoder, NS_OK);
    NS_DispatchToMainThread(event);
  }
}

nsresult FileMediaResource::GetCachedRanges(nsTArray<MediaByteRange>& aRanges)
{
  MutexAutoLock lock(mLock);

  EnsureSizeInitialized();
  if (mSize == -1) {
    return NS_ERROR_FAILURE;
  }
  aRanges.AppendElement(MediaByteRange(0, mSize));
  return NS_OK;
}

nsresult FileMediaResource::Open(nsIStreamListener** aStreamListener)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (aStreamListener) {
    *aStreamListener = nullptr;
  }

  nsresult rv = NS_OK;
  if (aStreamListener) {
    
    
    
    nsCOMPtr<nsIFileChannel> fc(do_QueryInterface(mChannel));
    if (fc) {
      nsCOMPtr<nsIFile> file;
      rv = fc->GetFile(getter_AddRefs(file));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_NewLocalFileInputStream(getter_AddRefs(mInput), file);
    } else if (IsBlobURI(mURI)) {
      rv = NS_GetStreamForBlobURI(mURI, getter_AddRefs(mInput));
    }
  } else {
    
    
    MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
    NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);
    dom::HTMLMediaElement* element = owner->GetMediaElement();
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

  return NS_OK;
}

nsresult FileMediaResource::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  
  
  if (mChannel) {
    mChannel->Cancel(NS_ERROR_PARSED_DATA_CACHED);
    mChannel = nullptr;
  }

  return NS_OK;
}

already_AddRefed<nsIPrincipal> FileMediaResource::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIPrincipal> principal;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (!secMan || !mChannel)
    return nullptr;
  secMan->GetChannelResultPrincipal(mChannel, getter_AddRefs(principal));
  return principal.forget();
}

bool FileMediaResource::CanClone()
{
  return true;
}

already_AddRefed<MediaResource> FileMediaResource::CloneData(MediaDecoder* aDecoder)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  if (!owner) {
    
    return nullptr;
  }
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  if (!element) {
    
    return nullptr;
  }
  nsCOMPtr<nsILoadGroup> loadGroup = element->GetDocumentLoadGroup();
  NS_ENSURE_TRUE(loadGroup, nullptr);

  nsSecurityFlags securityFlags = nsILoadInfo::SEC_NORMAL;
  if (nsContentUtils::ChannelShouldInheritPrincipal(element->NodePrincipal(),
                                                    mURI,
                                                    false, 
                                                    false 
                                                    )) {
    securityFlags = nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL;
  }

  nsCOMPtr<nsIChannel> channel;
  nsresult rv =
    NS_NewChannel(getter_AddRefs(channel),
                  mURI,
                  element,
                  securityFlags,
                  nsIContentPolicy::TYPE_MEDIA,
                  loadGroup);

  if (NS_FAILED(rv))
    return nullptr;

  nsRefPtr<MediaResource> resource(new FileMediaResource(aDecoder, channel, mURI, GetContentType()));
  return resource.forget();
}

nsresult FileMediaResource::ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount)
{
  MutexAutoLock lock(mLock);

  EnsureSizeInitialized();
  if (!aCount) {
    return NS_OK;
  }
  int64_t offset = 0;
  nsresult res = mSeekable->Tell(&offset);
  NS_ENSURE_SUCCESS(res,res);
  res = mSeekable->Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
  NS_ENSURE_SUCCESS(res,res);
  uint32_t bytesRead = 0;
  do {
    uint32_t x = 0;
    uint32_t bytesToRead = aCount - bytesRead;
    res = mInput->Read(aBuffer, bytesToRead, &x);
    bytesRead += x;
    if (!x) {
      res = NS_ERROR_FAILURE;
    }
  } while (bytesRead != aCount && res == NS_OK);

  
  
  nsresult seekres = mSeekable->Seek(nsISeekableStream::NS_SEEK_SET, offset);

  
  NS_ENSURE_SUCCESS(res,res);

  
  return seekres;
}

nsresult FileMediaResource::Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes)
{
  nsresult rv;
  int64_t offset = 0;
  {
    MutexAutoLock lock(mLock);
    mSeekable->Tell(&offset);
    rv = UnsafeRead(aBuffer, aCount, aBytes);
  }
  if (NS_SUCCEEDED(rv)) {
    DispatchBytesConsumed(*aBytes, offset);
  }
  return rv;
}

nsresult FileMediaResource::UnsafeRead(char* aBuffer, uint32_t aCount, uint32_t* aBytes)
{
  EnsureSizeInitialized();
  return mInput->Read(aBuffer, aCount, aBytes);
}

nsresult FileMediaResource::ReadAt(int64_t aOffset, char* aBuffer,
                                   uint32_t aCount, uint32_t* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  nsresult rv;
  {
    MutexAutoLock lock(mLock);
    rv = UnsafeSeek(nsISeekableStream::NS_SEEK_SET, aOffset);
    if (NS_FAILED(rv)) return rv;
    rv = UnsafeRead(aBuffer, aCount, aBytes);
  }
  if (NS_SUCCEEDED(rv)) {
    DispatchBytesConsumed(*aBytes, aOffset);
  }
  return rv;
}

nsresult FileMediaResource::Seek(int32_t aWhence, int64_t aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  MutexAutoLock lock(mLock);
  return UnsafeSeek(aWhence, aOffset);
}

nsresult FileMediaResource::UnsafeSeek(int32_t aWhence, int64_t aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  if (!mSeekable)
    return NS_ERROR_FAILURE;
  EnsureSizeInitialized();
  return mSeekable->Seek(aWhence, aOffset);
}

int64_t FileMediaResource::Tell()
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  MutexAutoLock lock(mLock);
  EnsureSizeInitialized();

  int64_t offset = 0;
  
  if (!mSeekable || NS_FAILED(mSeekable->Tell(&offset)))
    return mSize;
  return offset;
}

already_AddRefed<MediaResource>
MediaResource::Create(MediaDecoder* aDecoder, nsIChannel* aChannel)
{
  NS_ASSERTION(NS_IsMainThread(),
               "MediaResource::Open called on non-main thread");

  
  
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, nullptr);

  nsAutoCString contentType;
  aChannel->GetContentType(contentType);

  nsCOMPtr<nsIFileChannel> fc = do_QueryInterface(aChannel);
  nsRefPtr<MediaResource> resource;
  if (fc || IsBlobURI(uri)) {
    resource = new FileMediaResource(aDecoder, aChannel, uri, contentType);
  } else if (IsRtspURI(uri)) {
    resource = new RtspMediaResource(aDecoder, aChannel, uri, contentType);
  } else {
    resource = new ChannelMediaResource(aDecoder, aChannel, uri, contentType);
  }
  return resource.forget();
}

void BaseMediaResource::SetLoadInBackground(bool aLoadInBackground) {
  if (aLoadInBackground == mLoadInBackground) {
    return;
  }
  mLoadInBackground = aLoadInBackground;
  if (!mChannel) {
    
    return;
  }

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  if (!owner) {
    NS_WARNING("Null owner in MediaResource::SetLoadInBackground()");
    return;
  }
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  if (!element) {
    NS_WARNING("Null element in MediaResource::SetLoadInBackground()");
    return;
  }

  bool isPending = false;
  if (NS_SUCCEEDED(mChannel->IsPending(&isPending)) &&
      isPending) {
    nsLoadFlags loadFlags;
    DebugOnly<nsresult> rv = mChannel->GetLoadFlags(&loadFlags);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadFlags() failed!");

    if (aLoadInBackground) {
      loadFlags |= nsIRequest::LOAD_BACKGROUND;
    } else {
      loadFlags &= ~nsIRequest::LOAD_BACKGROUND;
    }
    ModifyLoadFlags(loadFlags);
  }
}

void BaseMediaResource::ModifyLoadFlags(nsLoadFlags aFlags)
{
  nsCOMPtr<nsILoadGroup> loadGroup;
  DebugOnly<nsresult> rv = mChannel->GetLoadGroup(getter_AddRefs(loadGroup));
  NS_ASSERTION(NS_SUCCEEDED(rv), "GetLoadGroup() failed!");

  nsresult status;
  mChannel->GetStatus(&status);

  
  if (loadGroup &&
      NS_SUCCEEDED(status)) {
    rv = loadGroup->RemoveRequest(mChannel, nullptr, status);
    NS_ASSERTION(NS_SUCCEEDED(rv), "RemoveRequest() failed!");
  }

  rv = mChannel->SetLoadFlags(aFlags);
  NS_ASSERTION(NS_SUCCEEDED(rv), "SetLoadFlags() failed!");

  if (loadGroup &&
      NS_SUCCEEDED(status)) {
    rv = loadGroup->AddRequest(mChannel, nullptr);
    NS_ASSERTION(NS_SUCCEEDED(rv), "AddRequest() failed!");
  }
}

class DispatchBytesConsumedEvent : public nsRunnable {
public:
  DispatchBytesConsumedEvent(MediaDecoder* aDecoder,
                             int64_t aNumBytes,
                             int64_t aOffset)
    : mDecoder(aDecoder),
      mNumBytes(aNumBytes),
      mOffset(aOffset)
  {
    MOZ_COUNT_CTOR(DispatchBytesConsumedEvent);
  }

protected:
  ~DispatchBytesConsumedEvent()
  {
    MOZ_COUNT_DTOR(DispatchBytesConsumedEvent);
  }

public:
  NS_IMETHOD Run() {
    mDecoder->NotifyBytesConsumed(mNumBytes, mOffset);
    
    
    mDecoder = nullptr;
    return NS_OK;
  }

  RefPtr<MediaDecoder> mDecoder;
  int64_t mNumBytes;
  int64_t mOffset;
};

void BaseMediaResource::DispatchBytesConsumed(int64_t aNumBytes, int64_t aOffset)
{
  if (aNumBytes <= 0) {
    return;
  }
  RefPtr<nsIRunnable> event(new DispatchBytesConsumedEvent(mDecoder, aNumBytes, aOffset));
  NS_DispatchToMainThread(event);
}

} 

