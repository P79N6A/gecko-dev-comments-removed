





#include "MediaSource.h"

#include "mozilla/dom/HTMLMediaElement.h"
#include "MediaSourceInputAdapter.h"
#include "SourceBuffer.h"
#include "SourceBufferList.h"
#include "nsContentTypeParser.h"
#include "nsIInputStream.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaSourceLog;
#define LOG(type, msg) PR_LOG(gMediaSourceLog, type, msg)
#else
#define LOG(type, msg)
#endif

namespace mozilla {
namespace dom {

already_AddRefed<nsIInputStream>
MediaSource::CreateInternalStream()
{
  nsRefPtr<MediaSourceInputAdapter> adapter = new MediaSourceInputAdapter(this);
  mAdapters.AppendElement(adapter);
  return adapter.forget();
}

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
    return UnspecifiedNaN();
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
  if (!IsTypeSupportedInternal(aType, aRv)) {
    return nullptr;
  }
  
  if (mSourceBuffers->Length() >= 1) {
    aRv.Throw(NS_ERROR_DOM_QUOTA_EXCEEDED_ERR);
    return nullptr;
  }
  if (mReadyState != MediaSourceReadyState::Open) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }
  mContentType = aType;
  nsRefPtr<SourceBuffer> sourceBuffer = new SourceBuffer(this);
  mSourceBuffers->Append(sourceBuffer);
  sourceBuffer->Attach();
  return sourceBuffer.forget();
}

void
MediaSource::RemoveSourceBuffer(SourceBuffer& aSourceBuffer, ErrorResult& aRv)
{
  SourceBuffer* sourceBuffer = &aSourceBuffer;
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
  sourceBuffer->Detach();
  
}

void
MediaSource::EndOfStream(const Optional<MediaSourceEndOfStreamError>& aError, ErrorResult& aRv)
{
  if (mReadyState != MediaSourceReadyState::Open ||
      mSourceBuffers->AnyUpdating()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  EndOfStreamInternal(aError, aRv);
}

 bool
MediaSource::IsTypeSupported(const GlobalObject& aGlobal,
                             const nsAString& aType)
{
  ErrorResult unused;
  return IsTypeSupportedInternal(aType, unused);
}

void
MediaSource::AppendData(const uint8_t* aData, uint32_t aLength, ErrorResult& aRv)
{
  MonitorAutoLock mon(mMonitor);
  LOG(PR_LOG_DEBUG, ("%p Append(ArrayBuffer=%u) mData=%u", this, aLength, mData.Length()));
  mData.AppendElements(aData, aLength);
  NotifyListeners();
}

bool
MediaSource::AttachElement(HTMLMediaElement* aElement)
{
  LOG(PR_LOG_DEBUG, ("%p Attaching element %p", this, aElement));
  MOZ_ASSERT(aElement);
  mElement = aElement;
  if (mReadyState != MediaSourceReadyState::Closed) {
    return false;
  }
  SetReadyState(MediaSourceReadyState::Open);
  return true;
}

void
MediaSource::DetachElement()
{
  LOG(PR_LOG_DEBUG, ("%p Detaching element %p", this, mElement.get()));
  MOZ_ASSERT(mElement);
  mElement = nullptr;
  mDuration = UnspecifiedNaN();
  mActiveSourceBuffers->Clear();
  mSourceBuffers->DetachAndClear();
  SetReadyState(MediaSourceReadyState::Closed);

  for (uint32_t i = 0; i < mAdapters.Length(); ++i) {
    mAdapters[i]->Close();
  }
  mAdapters.Clear();
}

MediaSource::MediaSource(nsPIDOMWindow* aWindow)
  : nsDOMEventTargetHelper(aWindow)
  , mDuration(UnspecifiedNaN())
  , mMonitor("mozilla::dom::MediaSource::mMonitor")
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
  MonitorAutoLock mon(mMonitor);

  NotifyListeners();

  if ((mReadyState == MediaSourceReadyState::Closed ||
       mReadyState == MediaSourceReadyState::Ended) &&
      aState == MediaSourceReadyState::Open) {
    mReadyState = aState;
    QueueAsyncSimpleEvent("sourceopen");
    return;
  }

  if (mReadyState == MediaSourceReadyState::Open &&
      aState == MediaSourceReadyState::Ended) {
    mReadyState = aState;
    QueueAsyncSimpleEvent("sourceended");
    return;
  }

  if ((mReadyState == MediaSourceReadyState::Open ||
       mReadyState == MediaSourceReadyState::Ended) &&
      aState == MediaSourceReadyState::Closed) {
    mReadyState = aState;
    QueueAsyncSimpleEvent("sourceclose");
    return;
  }

  NS_WARNING("Invalid MediaSource readyState transition");
}

void
MediaSource::GetBuffered(TimeRanges* aRanges)
{
  if (mActiveSourceBuffers->Length() == 0) {
    return;
  }
  
}

void
MediaSource::DispatchSimpleEvent(const char* aName)
{
  LOG(PR_LOG_DEBUG, ("%p Dispatching event %s to MediaSource", this, aName));
  DispatchTrustedEvent(NS_ConvertUTF8toUTF16(aName));
}

void
MediaSource::QueueAsyncSimpleEvent(const char* aName)
{
  LOG(PR_LOG_DEBUG, ("%p Queuing event %s to MediaSource", this, aName));
  nsCOMPtr<nsIRunnable> event = new AsyncEventRunner<MediaSource>(this, aName);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

void
MediaSource::NotifyListeners()
{
  for (uint32_t i = 0; i < mAdapters.Length(); ++i) {
    mAdapters[i]->NotifyListener();
  }
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

void
MediaSource::EndOfStreamInternal(const Optional<MediaSourceEndOfStreamError>& aError, ErrorResult& aRv)
{
  SetReadyState(MediaSourceReadyState::Ended);
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

static const char* const gMediaSourceTypes[5] = {
  "video/webm",
  "audio/webm",
  "video/mp4",
  "audio/mp4",
  nullptr
};

 bool
MediaSource::IsTypeSupportedInternal(const nsAString& aType, ErrorResult& aRv)
{
  if (aType.IsEmpty()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return false;
  }
  
  nsContentTypeParser parser(aType);
  nsAutoString mimeType;
  nsresult rv = parser.GetType(mimeType);
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return false;
  }
  bool found = false;
  for (uint32_t i = 0; gMediaSourceTypes[i]; ++i) {
    if (mimeType.EqualsASCII(gMediaSourceTypes[i])) {
      found = true;
      break;
    }
  }
  if (!found) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return false;
  }
  
  
  
  if (HTMLMediaElement::GetCanPlay(aType) == CANPLAY_NO) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return false;
  }
  return true;
}

nsPIDOMWindow*
MediaSource::GetParentObject() const
{
  return GetOwner();
}

JSObject*
MediaSource::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MediaSourceBinding::Wrap(aCx, aScope, this);
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_4(MediaSource, nsDOMEventTargetHelper,
                                     mSourceBuffers, mActiveSourceBuffers, mAdapters, mElement)

NS_IMPL_ADDREF_INHERITED(MediaSource, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaSource, nsDOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaSource)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::MediaSource)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

} 
} 
