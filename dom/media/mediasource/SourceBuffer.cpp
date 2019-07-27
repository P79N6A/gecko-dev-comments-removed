





#include "SourceBuffer.h"

#include "AsyncEventRunner.h"
#include "MediaSourceUtils.h"
#include "TrackBuffer.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/MediaSourceBinding.h"
#include "mozilla/dom/TimeRanges.h"
#include "nsError.h"
#include "nsIEventTarget.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"
#include "prlog.h"

struct JSContext;
class JSObject;

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaSourceLog();
extern PRLogModuleInfo* GetMediaSourceAPILog();

#define MSE_DEBUG(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#define MSE_DEBUGV(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG+1, (__VA_ARGS__))
#define MSE_API(...) PR_LOG(GetMediaSourceAPILog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#define MSE_DEBUGV(...)
#define MSE_API(...)
#endif

namespace mozilla {

namespace dom {

void
SourceBuffer::SetMode(SourceBufferAppendMode aMode, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::SetMode(aMode=%d)", this, aMode);
  if (!IsAttached() || mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (aMode == SourceBufferAppendMode::Sequence) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }
  MOZ_ASSERT(mMediaSource->ReadyState() != MediaSourceReadyState::Closed);
  if (mMediaSource->ReadyState() == MediaSourceReadyState::Ended) {
    mMediaSource->SetReadyState(MediaSourceReadyState::Open);
  }
  
  
  mAppendMode = aMode;
}

void
SourceBuffer::SetTimestampOffset(double aTimestampOffset, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::SetTimestampOffset(aTimestampOffset=%f)", this, aTimestampOffset);
  if (!IsAttached() || mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  MOZ_ASSERT(mMediaSource->ReadyState() != MediaSourceReadyState::Closed);
  if (mMediaSource->ReadyState() == MediaSourceReadyState::Ended) {
    mMediaSource->SetReadyState(MediaSourceReadyState::Open);
  }
  
  
  mTimestampOffset = aTimestampOffset;
}

already_AddRefed<TimeRanges>
SourceBuffer::GetBuffered(ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!IsAttached()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }
  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  double highestEndTime = mTrackBuffer->Buffered(ranges);
  if (mMediaSource->ReadyState() == MediaSourceReadyState::Ended) {
    
    
    
    ranges->Add(ranges->GetEndTime(), highestEndTime);
    ranges->Normalize();
  }
  MSE_DEBUGV("SourceBuffer(%p)::GetBuffered ranges=%s", this, DumpTimeRanges(ranges).get());
  return ranges.forget();
}

void
SourceBuffer::SetAppendWindowStart(double aAppendWindowStart, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::SetAppendWindowStart(aAppendWindowStart=%f)", this, aAppendWindowStart);
  if (!IsAttached() || mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (aAppendWindowStart < 0 || aAppendWindowStart >= mAppendWindowEnd) {
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }
  mAppendWindowStart = aAppendWindowStart;
}

void
SourceBuffer::SetAppendWindowEnd(double aAppendWindowEnd, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::SetAppendWindowEnd(aAppendWindowEnd=%f)", this, aAppendWindowEnd);
  if (!IsAttached() || mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (IsNaN(aAppendWindowEnd) ||
      aAppendWindowEnd <= mAppendWindowStart) {
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }
  mAppendWindowEnd = aAppendWindowEnd;
}

void
SourceBuffer::AppendBuffer(const ArrayBuffer& aData, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::AppendBuffer(ArrayBuffer)", this);
  aData.ComputeLengthAndData();
  AppendData(aData.Data(), aData.Length(), aRv);
}

void
SourceBuffer::AppendBuffer(const ArrayBufferView& aData, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::AppendBuffer(ArrayBufferView)", this);
  aData.ComputeLengthAndData();
  AppendData(aData.Data(), aData.Length(), aRv);
}

void
SourceBuffer::Abort(ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::Abort()", this);
  if (!IsAttached()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (mMediaSource->ReadyState() != MediaSourceReadyState::Open) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (mUpdating) {
    
    AbortUpdating();
  }
  
  mAppendWindowStart = 0;
  mAppendWindowEnd = PositiveInfinity<double>();

  MSE_DEBUG("SourceBuffer(%p)::Abort() Discarding decoder", this);
  mTrackBuffer->DiscardDecoder();
}

void
SourceBuffer::Remove(double aStart, double aEnd, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p)::Remove(aStart=%f, aEnd=%f)", this, aStart, aEnd);
  if (!IsAttached()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (IsNaN(mMediaSource->Duration()) ||
      aStart < 0 || aStart > mMediaSource->Duration() ||
      aEnd <= aStart || IsNaN(aEnd)) {
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }
  if (mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (mMediaSource->ReadyState() == MediaSourceReadyState::Ended) {
    mMediaSource->SetReadyState(MediaSourceReadyState::Open);
  }
  RangeRemoval(aStart, aEnd);
}

void
SourceBuffer::RangeRemoval(double aStart, double aEnd)
{
  StartUpdating();
  

  
  
  
  nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &SourceBuffer::StopUpdating);
  NS_DispatchToMainThread(event);
}

void
SourceBuffer::Detach()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("SourceBuffer(%p)::Detach", this);
  if (mTrackBuffer) {
    mTrackBuffer->Detach();
  }
  mTrackBuffer = nullptr;
  mMediaSource = nullptr;
}

void
SourceBuffer::Ended()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(IsAttached());
  MSE_DEBUG("SourceBuffer(%p)::Ended", this);
  mTrackBuffer->DiscardDecoder();
}

SourceBuffer::SourceBuffer(MediaSource* aMediaSource, const nsACString& aType)
  : DOMEventTargetHelper(aMediaSource->GetParentObject())
  , mMediaSource(aMediaSource)
  , mAppendWindowStart(0)
  , mAppendWindowEnd(PositiveInfinity<double>())
  , mTimestampOffset(0)
  , mAppendMode(SourceBufferAppendMode::Segments)
  , mUpdating(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aMediaSource);
  mEvictionThreshold = Preferences::GetUint("media.mediasource.eviction_threshold",
                                            75 * (1 << 20));
  mTrackBuffer = new TrackBuffer(aMediaSource->GetDecoder(), aType);
  MSE_DEBUG("SourceBuffer(%p)::SourceBuffer: Create mTrackBuffer=%p",
            this, mTrackBuffer.get());
}

SourceBuffer::~SourceBuffer()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mMediaSource);
  MSE_DEBUG("SourceBuffer(%p)::~SourceBuffer", this);
}

MediaSource*
SourceBuffer::GetParentObject() const
{
  return mMediaSource;
}

JSObject*
SourceBuffer::WrapObject(JSContext* aCx)
{
  return SourceBufferBinding::Wrap(aCx, this);
}

void
SourceBuffer::DispatchSimpleEvent(const char* aName)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_API("SourceBuffer(%p) Dispatch event '%s'", this, aName);
  DispatchTrustedEvent(NS_ConvertUTF8toUTF16(aName));
}

void
SourceBuffer::QueueAsyncSimpleEvent(const char* aName)
{
  MSE_DEBUG("SourceBuffer(%p) Queuing event '%s'", this, aName);
  nsCOMPtr<nsIRunnable> event = new AsyncEventRunner<SourceBuffer>(this, aName);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

void
SourceBuffer::StartUpdating()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mUpdating);
  mUpdating = true;
  QueueAsyncSimpleEvent("updatestart");
}

void
SourceBuffer::StopUpdating()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mUpdating) {
    
    
    
    
    
    
    return;
  }
  mUpdating = false;
  QueueAsyncSimpleEvent("update");
  QueueAsyncSimpleEvent("updateend");
}

void
SourceBuffer::AbortUpdating()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mUpdating);
  mUpdating = false;
  QueueAsyncSimpleEvent("abort");
  QueueAsyncSimpleEvent("updateend");
}

void
SourceBuffer::CheckEndTime()
{
  
  double endTime = GetBufferedEnd();
  if (endTime > mMediaSource->Duration()) {
    mMediaSource->SetDuration(endTime);
  }
}

void
SourceBuffer::AppendData(const uint8_t* aData, uint32_t aLength, ErrorResult& aRv)
{
  MSE_DEBUG("SourceBuffer(%p)::AppendData(aLength=%u)", this, aLength);
  if (!PrepareAppend(aRv)) {
    return;
  }
  StartUpdating();

  MOZ_ASSERT(mAppendMode == SourceBufferAppendMode::Segments,
             "We don't handle timestampOffset for sequence mode yet");
  if (aLength) {
    if (!mTrackBuffer->AppendData(aData, aLength, mTimestampOffset * USECS_PER_S)) {
      nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethodWithArg<bool>(this, &SourceBuffer::AppendError, true);
      NS_DispatchToMainThread(event);
      return;
    }

    if (mTrackBuffer->HasInitSegment()) {
      mMediaSource->QueueInitializationEvent();
    }

    CheckEndTime();
  }

  
  
  
  nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &SourceBuffer::StopUpdating);
  NS_DispatchToMainThread(event);
}

void
SourceBuffer::AppendError(bool aDecoderError)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mUpdating) {
    
    return;
  }
  mTrackBuffer->ResetParserState();

  mUpdating = false;

  QueueAsyncSimpleEvent("error");
  QueueAsyncSimpleEvent("updateend");

  if (aDecoderError) {
    Optional<MediaSourceEndOfStreamError> decodeError(
      MediaSourceEndOfStreamError::Decode);
    ErrorResult dummy;
    mMediaSource->EndOfStream(decodeError, dummy);
  }
}

bool
SourceBuffer::PrepareAppend(ErrorResult& aRv)
{
  if (!IsAttached() || mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return false;
  }
  if (mMediaSource->ReadyState() == MediaSourceReadyState::Ended) {
    mMediaSource->SetReadyState(MediaSourceReadyState::Open);
  }

  
  
  
  
  
  
  
  
  double newBufferStartTime = 0.0;
  bool evicted =
    mTrackBuffer->EvictData(mMediaSource->GetDecoder()->GetCurrentTime(),
                            mEvictionThreshold, &newBufferStartTime);
  if (evicted) {
    MSE_DEBUG("SourceBuffer(%p)::AppendData Evict; current buffered start=%f",
              this, GetBufferedStart());

    
    
    mMediaSource->NotifyEvicted(0.0, newBufferStartTime);
  }

  
  return true;
}

double
SourceBuffer::GetBufferedStart()
{
  MOZ_ASSERT(NS_IsMainThread());
  ErrorResult dummy;
  nsRefPtr<TimeRanges> ranges = GetBuffered(dummy);
  return ranges->Length() > 0 ? ranges->GetStartTime() : 0;
}

double
SourceBuffer::GetBufferedEnd()
{
  MOZ_ASSERT(NS_IsMainThread());
  ErrorResult dummy;
  nsRefPtr<TimeRanges> ranges = GetBuffered(dummy);
  return ranges->Length() > 0 ? ranges->GetEndTime() : 0;
}

void
SourceBuffer::Evict(double aStart, double aEnd)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("SourceBuffer(%p)::Evict(aStart=%f, aEnd=%f)", this, aStart, aEnd);
  double currentTime = mMediaSource->GetDecoder()->GetCurrentTime();
  double evictTime = aEnd;
  const double safety_threshold = 5;
  if (currentTime + safety_threshold >= evictTime) {
    evictTime -= safety_threshold;
  }
  mTrackBuffer->EvictBefore(evictTime);
}

#if defined(DEBUG)
void
SourceBuffer::Dump(const char* aPath)
{
  if (mTrackBuffer) {
    mTrackBuffer->Dump(aPath);
  }
}
#endif

NS_IMPL_CYCLE_COLLECTION_CLASS(SourceBuffer)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(SourceBuffer)
  
  TrackBuffer* track = tmp->mTrackBuffer;
  if (track) {
    track->Detach();
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mMediaSource)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END_INHERITED(DOMEventTargetHelper)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(SourceBuffer,
                                                  DOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMediaSource)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(SourceBuffer, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(SourceBuffer, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(SourceBuffer)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

} 

} 
