




#include "WebVTTListener.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/dom/TextTrackRegion.h"
#include "mozilla/dom/VTTRegionBinding.h"
#include "mozilla/dom/HTMLTrackElement.h"
#include "nsIInputStream.h"
#include "nsIWebVTTParserWrapper.h"
#include "nsComponentManagerUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_2(WebVTTListener, mElement, mParserWrapper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebVTTListener)
  NS_INTERFACE_MAP_ENTRY(nsIWebVTTListener)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebVTTListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebVTTListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebVTTListener)

#ifdef PR_LOGGING
PRLogModuleInfo* gTextTrackLog;
# define LOG(...) PR_LOG(gTextTrackLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
# define LOG(msg)
#endif

WebVTTListener::WebVTTListener(HTMLTrackElement* aElement)
  : mElement(aElement)
{
  MOZ_ASSERT(mElement, "Must pass an element to the callback");
#ifdef PR_LOGGING
  if (!gTextTrackLog) {
    gTextTrackLog = PR_NewLogModule("TextTrack");
  }
#endif
  LOG("WebVTTListener created.");
}

WebVTTListener::~WebVTTListener()
{
  LOG("WebVTTListener destroyed.");
}

NS_IMETHODIMP
WebVTTListener::GetInterface(const nsIID &aIID,
                             void** aResult)
{
  return QueryInterface(aIID, aResult);
}

nsresult
WebVTTListener::LoadResource()
{
  if (!HTMLTrackElement::IsWebVTTEnabled()) {
    NS_WARNING("WebVTT support disabled."
               " See media.webvtt.enabled in about:config. ");
    return NS_ERROR_FAILURE;
  }
  nsresult rv;
  mParserWrapper = do_CreateInstance(NS_WEBVTTPARSERWRAPPER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsPIDOMWindow* window = mElement->OwnerDoc()->GetWindow();
  rv = mParserWrapper->LoadParser(window);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mParserWrapper->Watch(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mElement->mTrack->SetReadyState(HTMLTrackElement::LOADING);
  return NS_OK;
}

NS_IMETHODIMP
WebVTTListener::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
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
WebVTTListener::OnStartRequest(nsIRequest* aRequest,
                               nsISupports* aContext)
{
  return NS_OK;
}

NS_IMETHODIMP
WebVTTListener::OnStopRequest(nsIRequest* aRequest,
                              nsISupports* aContext,
                              nsresult aStatus)
{
  if (mElement->ReadyState() != HTMLTrackElement::ERROR) {
    TextTrack* track = mElement->Track();
    track->SetReadyState(HTMLTrackElement::LOADED);
  }
  
  mParserWrapper->Flush();
  return NS_OK;
}

NS_METHOD
WebVTTListener::ParseChunk(nsIInputStream* aInStream, void* aClosure,
                           const char* aFromSegment, uint32_t aToOffset,
                           uint32_t aCount, uint32_t* aWriteCount)
{
  nsCString buffer(aFromSegment, aCount);
  WebVTTListener* listener = static_cast<WebVTTListener*>(aClosure);

  if (NS_FAILED(listener->mParserWrapper->Parse(buffer))) {
    LOG("Unable to parse chunk of WEBVTT text. Aborting.");
    *aWriteCount = 0;
    return NS_ERROR_FAILURE;
  }

  *aWriteCount = aCount;
  return NS_OK;
}

NS_IMETHODIMP
WebVTTListener::OnDataAvailable(nsIRequest* aRequest,
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
WebVTTListener::OnCue(const JS::Value &aCue, JSContext* aCx)
{
  if (!aCue.isObject()) {
    return NS_ERROR_FAILURE;
  }

  TextTrackCue* cue;
  nsresult rv = UNWRAP_OBJECT(VTTCue, aCx, &aCue.toObject(), cue);
  NS_ENSURE_SUCCESS(rv, rv);

  cue->SetTrackElement(mElement);
  mElement->mTrack->AddCue(*cue);

  return NS_OK;
}


NS_IMETHODIMP
WebVTTListener::OnRegion(const JS::Value &aRegion, JSContext* aCx)
{
  if (!aRegion.isObject()) {
    return NS_ERROR_FAILURE;
  }

  TextTrackRegion* region;
  nsresult rv = UNWRAP_OBJECT(VTTRegion, aCx, &aRegion.toObject(),
                              region);
  NS_ENSURE_SUCCESS(rv, rv);

  mElement->mTrack->AddRegion(*region);

  return NS_OK;
}

} 
} 
