





#include "MediaSource.h"

#include "AsyncEventRunner.h"
#include "DecoderTraits.h"
#include "SourceBuffer.h"
#include "SourceBufferList.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/mozalloc.h"
#include "nsContentTypeParser.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsIEventTarget.h"
#include "nsIRunnable.h"
#include "nsPIDOMWindow.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"
#include "prlog.h"

struct JSContext;
class JSObject;

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaSourceLog;
#define MSE_DEBUG(...) PR_LOG(gMediaSourceLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#endif


static const unsigned int MAX_SOURCE_BUFFERS = 16;

namespace mozilla {

static const char* const gMediaSourceTypes[6] = {
  "video/webm",
  "audio/webm",



#if 0
  "video/mp4",
  "audio/mp4",
  "audio/mpeg",
#endif
  nullptr
};

static nsresult
IsTypeSupported(const nsAString& aType)
{
  if (aType.IsEmpty()) {
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }
  
  nsContentTypeParser parser(aType);
  nsAutoString mimeType;
  nsresult rv = parser.GetType(mimeType);
  if (NS_FAILED(rv)) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
  bool found = false;
  for (uint32_t i = 0; gMediaSourceTypes[i]; ++i) {
    if (mimeType.EqualsASCII(gMediaSourceTypes[i])) {
      found = true;
      break;
    }
  }
  if (!found) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
  if (Preferences::GetBool("media.mediasource.ignore_codecs", false)) {
    return NS_OK;
  }
  
  
  
  if (dom::HTMLMediaElement::GetCanPlay(aType) == CANPLAY_NO) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
  return NS_OK;
}

namespace dom {

 already_AddRefed<MediaSource>
MediaSource::Constructor(const GlobalObject& aGlobal,
                         ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<MediaSource> mediaSource = new MediaSource(window);
  return mediaSource.forget();
}

SourceBufferList*
MediaSource::SourceBuffers()
{
  MOZ_ASSERT_IF(mReadyState == MediaSourceReadyState::Closed, mSourceBuffers->IsEmpty());
  return mSourceBuffers;
}

SourceBufferList*
MediaSource::ActiveSourceBuffers()
{
  MOZ_ASSERT_IF(mReadyState == MediaSourceReadyState::Closed, mActiveSourceBuffers->IsEmpty());
  return mActiveSourceBuffers;
}

MediaSourceReadyState
MediaSource::ReadyState()
{
  return mReadyState;
}

double
MediaSource::Duration()
{
  if (mReadyState == MediaSourceReadyState::Closed) {
    return UnspecifiedNaN<double>();
  }
  return mDuration;
}

void
MediaSource::SetDuration(double aDuration, ErrorResult& aRv)
{
  if (aDuration < 0 || IsNaN(aDuration)) {
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }
  if (mReadyState != MediaSourceReadyState::Open ||
      mSourceBuffers->AnyUpdating()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  DurationChange(aDuration, aRv);
}

already_AddRefed<SourceBuffer>
MediaSource::AddSourceBuffer(const nsAString& aType, ErrorResult& aRv)
{
  nsresult rv = mozilla::IsTypeSupported(aType);
  MSE_DEBUG("MediaSource::AddSourceBuffer(Type=%s) -> %x", NS_ConvertUTF16toUTF8(aType).get(), rv);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }
  if (mSourceBuffers->Length() >= MAX_SOURCE_BUFFERS) {
    aRv.Throw(NS_ERROR_DOM_QUOTA_EXCEEDED_ERR);
    return nullptr;
  }
  if (mReadyState != MediaSourceReadyState::Open) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }
  nsContentTypeParser parser(aType);
  nsAutoString mimeType;
  rv = parser.GetType(mimeType);
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }
  nsRefPtr<SourceBuffer> sourceBuffer = SourceBuffer::Create(this, NS_ConvertUTF16toUTF8(mimeType));
  if (!sourceBuffer) {
    aRv.Throw(NS_ERROR_FAILURE); 
    return nullptr;
  }
  mSourceBuffers->Append(sourceBuffer);
  mActiveSourceBuffers->Append(sourceBuffer);
  MSE_DEBUG("%p AddSourceBuffer(Type=%s) -> %p", this,
            NS_ConvertUTF16toUTF8(mimeType).get(), sourceBuffer.get());
  return sourceBuffer.forget();
}

void
MediaSource::RemoveSourceBuffer(SourceBuffer& aSourceBuffer, ErrorResult& aRv)
{
  SourceBuffer* sourceBuffer = &aSourceBuffer;
  MSE_DEBUG("%p RemoveSourceBuffer(Buffer=%p)", this, sourceBuffer);
  if (!mSourceBuffers->Contains(sourceBuffer)) {
    aRv.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
    return;
  }
  if (sourceBuffer->Updating()) {
    
    
    
    
    
  }
  
  
  
  
  
  
  
  if (mActiveSourceBuffers->Contains(sourceBuffer)) {
    mActiveSourceBuffers->Remove(sourceBuffer);
  }
  mSourceBuffers->Remove(sourceBuffer);
  
}

void
MediaSource::EndOfStream(const Optional<MediaSourceEndOfStreamError>& aError, ErrorResult& aRv)
{
  MSE_DEBUG("%p EndOfStream(Error=%u)", this, aError.WasPassed() ? uint32_t(aError.Value()) : 0);
  if (mReadyState != MediaSourceReadyState::Open ||
      mSourceBuffers->AnyUpdating()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  SetReadyState(MediaSourceReadyState::Ended);
  mSourceBuffers->Ended();
  if (!aError.WasPassed()) {
    
    
    
    
    
    
    
    return;
  }
  switch (aError.Value()) {
  case MediaSourceEndOfStreamError::Network:
    
    
    
    break;
  case MediaSourceEndOfStreamError::Decode:
    
    
    
    break;
  default:
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
  }
}

 bool
MediaSource::IsTypeSupported(const GlobalObject&, const nsAString& aType)
{
#ifdef PR_LOGGING
  if (!gMediaSourceLog) {
    gMediaSourceLog = PR_NewLogModule("MediaSource");
  }
#endif
  nsresult rv = mozilla::IsTypeSupported(aType);
  MSE_DEBUG("MediaSource::IsTypeSupported(Type=%s) -> %x", NS_ConvertUTF16toUTF8(aType).get(), rv);
  return NS_SUCCEEDED(rv);
}

bool
MediaSource::Attach(MediaSourceDecoder* aDecoder)
{
  MSE_DEBUG("%p Attaching decoder %p owner %p", this, aDecoder, aDecoder->GetOwner());
  MOZ_ASSERT(aDecoder);
  if (mReadyState != MediaSourceReadyState::Closed) {
    return false;
  }
  mDecoder = aDecoder;
  mDecoder->AttachMediaSource(this);
  SetReadyState(MediaSourceReadyState::Open);
  return true;
}

void
MediaSource::Detach()
{
  MSE_DEBUG("%p Detaching decoder %p owner %p", this, mDecoder.get(), mDecoder->GetOwner());
  MOZ_ASSERT(mDecoder);
  mDecoder->DetachMediaSource();
  mDecoder = nullptr;
  mDuration = UnspecifiedNaN<double>();
  mActiveSourceBuffers->Clear();
  mSourceBuffers->Clear();
  SetReadyState(MediaSourceReadyState::Closed);
}

MediaSource::MediaSource(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mDuration(UnspecifiedNaN<double>())
  , mDecoder(nullptr)
  , mReadyState(MediaSourceReadyState::Closed)
{
  mSourceBuffers = new SourceBufferList(this);
  mActiveSourceBuffers = new SourceBufferList(this);

#ifdef PR_LOGGING
  if (!gMediaSourceLog) {
    gMediaSourceLog = PR_NewLogModule("MediaSource");
  }
#endif
}

void
MediaSource::SetReadyState(MediaSourceReadyState aState)
{
  MOZ_ASSERT(aState != mReadyState);
  MSE_DEBUG("%p SetReadyState old=%d new=%d", this, mReadyState, aState);

  MediaSourceReadyState oldState = mReadyState;
  mReadyState = aState;

  if (mReadyState == MediaSourceReadyState::Open &&
      (oldState == MediaSourceReadyState::Closed ||
       oldState == MediaSourceReadyState::Ended)) {
    QueueAsyncSimpleEvent("sourceopen");
    return;
  }

  if (mReadyState == MediaSourceReadyState::Ended &&
      oldState == MediaSourceReadyState::Open) {
    QueueAsyncSimpleEvent("sourceended");
    return;
  }

  if (mReadyState == MediaSourceReadyState::Closed &&
      (oldState == MediaSourceReadyState::Open ||
       oldState == MediaSourceReadyState::Ended)) {
    QueueAsyncSimpleEvent("sourceclose");
    return;
  }

  NS_WARNING("Invalid MediaSource readyState transition");
}

void
MediaSource::DispatchSimpleEvent(const char* aName)
{
  MSE_DEBUG("%p Dispatching event %s to MediaSource", this, aName);
  DispatchTrustedEvent(NS_ConvertUTF8toUTF16(aName));
}

void
MediaSource::QueueAsyncSimpleEvent(const char* aName)
{
  MSE_DEBUG("%p Queuing event %s to MediaSource", this, aName);
  nsCOMPtr<nsIRunnable> event = new AsyncEventRunner<MediaSource>(this, aName);
  NS_DispatchToMainThread(event);
}

void
MediaSource::DurationChange(double aNewDuration, ErrorResult& aRv)
{
  if (mDuration == aNewDuration) {
    return;
  }
  double oldDuration = mDuration;
  mDuration = aNewDuration;
  if (aNewDuration < oldDuration) {
    mSourceBuffers->Remove(aNewDuration, oldDuration, aRv);
    if (aRv.Failed()) {
      return;
    }
  }
  
  
}

nsPIDOMWindow*
MediaSource::GetParentObject() const
{
  return GetOwner();
}

JSObject*
MediaSource::WrapObject(JSContext* aCx)
{
  return MediaSourceBinding::Wrap(aCx, this);
}

void
MediaSource::NotifyEvicted(double aStart, double aEnd)
{
  
  
  mSourceBuffers->Evict(aStart, aEnd);
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(MediaSource, DOMEventTargetHelper,
                                   mSourceBuffers, mActiveSourceBuffers)

NS_IMPL_ADDREF_INHERITED(MediaSource, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaSource, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaSource)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::MediaSource)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

} 

} 
