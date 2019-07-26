




#include "WebVTTLoadListener.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/dom/HTMLTrackElement.h"
#include "nsIInputStream.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_1(WebVTTLoadListener, mElement)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebVTTLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStreamListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebVTTLoadListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebVTTLoadListener)

#ifdef PR_LOGGING
PRLogModuleInfo* gTextTrackLog;
# define LOG(...) PR_LOG(gTextTrackLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
# define LOG(msg)
#endif

WebVTTLoadListener::WebVTTLoadListener(HTMLTrackElement* aElement)
  : mElement(aElement)
{
  MOZ_ASSERT(mElement, "Must pass an element to the callback");
#ifdef PR_LOGGING
  if (!gTextTrackLog) {
    gTextTrackLog = PR_NewLogModule("TextTrack");
  }
#endif
  LOG("WebVTTLoadListener created.");
}

WebVTTLoadListener::~WebVTTLoadListener()
{
  LOG("WebVTTLoadListener destroyed.");
}

nsresult
WebVTTLoadListener::LoadResource()
{
  if (!HTMLTrackElement::IsWebVTTEnabled()) {
    NS_WARNING("WebVTT support disabled."
               " See media.webvtt.enabled in about:config. ");
    return NS_ERROR_FAILURE;
  }

  LOG("Loading text track resource.");

  mElement->mReadyState = HTMLTrackElement::LOADING;
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::OnStartRequest(nsIRequest* aRequest,
                                   nsISupports* aContext)
{
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::OnStopRequest(nsIRequest* aRequest,
                                  nsISupports* aContext,
                                  nsresult aStatus)
{
  
  if(mElement->mReadyState != HTMLTrackElement::ERROR) {
    mElement->mReadyState = HTMLTrackElement::LOADED;
  }
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::OnDataAvailable(nsIRequest* aRequest,
                                    nsISupports* aContext,
                                    nsIInputStream* aStream,
                                    uint64_t aOffset,
                                    uint32_t aCount)
{
  uint32_t count = aCount;
  while (count > 0) {
    uint32_t read;
    nsresult rv = aStream->ReadSegments(ParseChunk, this, count, &read);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!read) {
      return NS_ERROR_FAILURE;
    }
    count -= read;
  }

  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                           nsIChannel* aNewChannel,
                                           uint32_t aFlags,
                                           nsIAsyncVerifyRedirectCallback* cb)
{
  if (mElement) {
    mElement->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
  }
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::GetInterface(const nsIID &aIID,
                                 void** aResult)
{
  return QueryInterface(aIID, aResult);
}

NS_METHOD
WebVTTLoadListener::ParseChunk(nsIInputStream* aInStream, void* aClosure,
                               const char* aFromSegment, uint32_t aToOffset,
                               uint32_t aCount, uint32_t* aWriteCount)
{
  
  if (1) {
    LOG("WebVTT parser disabled.");
    LOG("Unable to parse chunk of WEBVTT text. Aborting.");
    *aWriteCount = 0;
    return NS_ERROR_FAILURE;
  }
  *aWriteCount = aCount;
  return NS_OK;
}

} 
} 
