




#include "SourceBuffer.h"

#include "AsyncEventRunner.h"
#include "DecoderTraits.h"
#include "MediaDecoder.h"
#include "MediaSourceDecoder.h"
#include "MediaSourceUtils.h"
#include "SourceBufferResource.h"
#include "mozilla/Endian.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/dom/MediaSourceBinding.h"
#include "mozilla/dom/TimeRanges.h"
#include "mp4_demuxer/BufferStream.h"
#include "mp4_demuxer/MoofParser.h"
#include "nsError.h"
#include "nsIEventTarget.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"
#include "prlog.h"
#include "SourceBufferDecoder.h"
#include "mozilla/Preferences.h"

#include "WebMBufferedParser.h"

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

class ContainerParser {
public:
  virtual ~ContainerParser() {}

  virtual bool IsInitSegmentPresent(const uint8_t* aData, uint32_t aLength)
  {
    MSE_DEBUG("ContainerParser(%p)::IsInitSegmentPresent aLength=%u [%x%x%x%x]",
              this, aLength,
              aLength > 0 ? aData[0] : 0,
              aLength > 1 ? aData[1] : 0,
              aLength > 2 ? aData[2] : 0,
              aLength > 3 ? aData[3] : 0);
    return false;
  }

  virtual bool ParseStartAndEndTimestamps(const uint8_t* aData, uint32_t aLength,
                                          double& aStart, double& aEnd)
  {
    return false;
  }

  virtual const nsTArray<uint8_t>& InitData()
  {
    MOZ_ASSERT(mInitData.Length() > 0);
    return mInitData;
  }

  static ContainerParser* CreateForMIMEType(const nsACString& aType);

protected:
  nsTArray<uint8_t> mInitData;
};

class WebMContainerParser : public ContainerParser {
public:
  WebMContainerParser()
    : mTimecodeScale(0)
  {}

  bool IsInitSegmentPresent(const uint8_t* aData, uint32_t aLength)
  {
    ContainerParser::IsInitSegmentPresent(aData, aLength);
    
    
    
    
    
    
    
    
    
    
    if (aLength >= 4 &&
        aData[0] == 0x1a && aData[1] == 0x45 && aData[2] == 0xdf && aData[3] == 0xa3) {
      return true;
    }
    return false;
  }

  virtual bool ParseStartAndEndTimestamps(const uint8_t* aData, uint32_t aLength,
                                          double& aStart, double& aEnd)
  {
    
    
    

    WebMBufferedParser parser(0);
    if (mTimecodeScale != 0) {
      parser.SetTimecodeScale(mTimecodeScale);
    }

    nsTArray<WebMTimeDataOffset> mapping;
    ReentrantMonitor dummy("dummy");
    parser.Append(aData, aLength, mapping, dummy);

    mTimecodeScale = parser.GetTimecodeScale();

    
    
    
    if (IsInitSegmentPresent(aData, aLength)) {
      uint32_t length = aLength;
      if (!mapping.IsEmpty()) {
        length = mapping[0].mSyncOffset;
        MOZ_ASSERT(length <= aLength);
      }
      MSE_DEBUG("WebMContainerParser(%p)::ParseStartAndEndTimestamps: Stashed init of %u bytes.",
                this, length);

      mInitData.ReplaceElementsAt(0, mInitData.Length(), aData, length);
    }

    if (mapping.IsEmpty()) {
      return false;
    }

    static const double NS_PER_S = 1e9;
    aStart = mapping[0].mTimecode / NS_PER_S;
    aEnd = mapping.LastElement().mTimecode / NS_PER_S;

    MSE_DEBUG("WebMContainerParser(%p)::ParseStartAndEndTimestamps: [%f, %f] [fso=%lld, leo=%lld]",
              this, aStart, aEnd, mapping[0].mSyncOffset, mapping.LastElement().mEndOffset);

    return true;
  }

private:
  uint32_t mTimecodeScale;
};

class MP4ContainerParser : public ContainerParser {
public:
  MP4ContainerParser() : mTimescale(0) {}

  bool IsInitSegmentPresent(const uint8_t* aData, uint32_t aLength)
  {
    ContainerParser::IsInitSegmentPresent(aData, aLength);
    
    
    

    if (aLength < 8) {
      return false;
    }

    uint32_t chunk_size = BigEndian::readUint32(aData);
    if (chunk_size < 8) {
      return false;
    }

    if (Preferences::GetBool("media.mediasource.allow_init_moov", false)) {
      if (aData[4] == 'm' && aData[5] == 'o' && aData[6] == 'o' &&
          aData[7] == 'v') {
        return true;
      }
    }

    return aData[4] == 'f' && aData[5] == 't' && aData[6] == 'y' &&
           aData[7] == 'p';
  }

  virtual bool ParseStartAndEndTimestamps(const uint8_t* aData, uint32_t aLength,
                                          double& aStart, double& aEnd)
  {
    mp4_demuxer::MoofParser parser(new mp4_demuxer::BufferStream(aData, aLength), 0);
    parser.mMdhd.mTimescale = mTimescale;

    nsTArray<MediaByteRange> byteRanges;
    byteRanges.AppendElement(MediaByteRange(0, aLength));
    parser.RebuildFragmentedIndex(byteRanges);

    if (IsInitSegmentPresent(aData, aLength)) {
      const MediaByteRange& range = parser.mInitRange;
      MSE_DEBUG("MP4ContainerParser(%p)::ParseStartAndEndTimestamps: Stashed init of %u bytes.",
                this, range.mEnd - range.mStart);

      mInitData.ReplaceElementsAt(0, mInitData.Length(),
                                  aData + range.mStart,
                                  range.mEnd - range.mStart);
    }

    
    mTimescale = parser.mMdhd.mTimescale;

    mp4_demuxer::Interval<mp4_demuxer::Microseconds> compositionRange =
        parser.GetCompositionRange();

    if (compositionRange.IsNull()) {
      return false;
    }
    aStart = static_cast<double>(compositionRange.start) / USECS_PER_S;
    aEnd = static_cast<double>(compositionRange.end) / USECS_PER_S;
    MSE_DEBUG("MP4ContainerParser(%p)::ParseStartAndEndTimestamps: [%f, %f]",
              this, aStart, aEnd);
    return true;
  }

  private:
    uint32_t mTimescale;
};


 ContainerParser*
ContainerParser::CreateForMIMEType(const nsACString& aType)
{
  if (aType.LowerCaseEqualsLiteral("video/webm") || aType.LowerCaseEqualsLiteral("audio/webm")) {
    return new WebMContainerParser();
  }

  if (aType.LowerCaseEqualsLiteral("video/mp4") || aType.LowerCaseEqualsLiteral("audio/mp4")) {
    return new MP4ContainerParser();
  }
  return new ContainerParser();
}

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
  double highestEndTime = 0;
  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  
  
  
  for (uint32_t i = 0; i < mDecoders.Length(); ++i) {
    nsRefPtr<TimeRanges> r = new TimeRanges();
    mDecoders[i]->GetBuffered(r);
    if (r->Length() > 0) {
      highestEndTime = std::max(highestEndTime, r->GetEndTime());
      ranges->Union(r);
    }
  }
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
  DiscardDecoder();
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
  if (mUpdating || mMediaSource->ReadyState() != MediaSourceReadyState::Open) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  StartUpdating();
  
  StopUpdating();
}

void
SourceBuffer::Detach()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("SourceBuffer(%p)::Detach", this);
  Ended();
  DiscardDecoder();
  mMediaSource = nullptr;
}

void
SourceBuffer::Ended()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("SourceBuffer(%p)::Ended", this);
  if (mDecoder) {
    mDecoder->GetResource()->Ended();
  }
}

SourceBuffer::SourceBuffer(MediaSource* aMediaSource, const nsACString& aType)
  : DOMEventTargetHelper(aMediaSource->GetParentObject())
  , mMediaSource(aMediaSource)
  , mType(aType)
  , mLastParsedTimestamp(UnspecifiedNaN<double>())
  , mAppendWindowStart(0)
  , mAppendWindowEnd(PositiveInfinity<double>())
  , mTimestampOffset(0)
  , mAppendMode(SourceBufferAppendMode::Segments)
  , mUpdating(false)
  , mDecoderInitialized(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aMediaSource);
  mParser = ContainerParser::CreateForMIMEType(aType);
  MSE_DEBUG("SourceBuffer(%p)::SourceBuffer: Creating initial decoder, mParser=%p", this, mParser.get());
  InitNewDecoder();
}

SourceBuffer::~SourceBuffer()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("SourceBuffer(%p)::~SourceBuffer", this);
  DiscardDecoder();
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

bool
SourceBuffer::InitNewDecoder()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("SourceBuffer(%p)::InitNewDecoder", this);
  MOZ_ASSERT(!mDecoder);
  MediaSourceDecoder* parentDecoder = mMediaSource->GetDecoder();
  nsRefPtr<SourceBufferDecoder> decoder = parentDecoder->CreateSubDecoder(mType);
  if (!decoder) {
    return false;
  }
  mDecoder = decoder;
  mDecoderInitialized = false;
  mDecoders.AppendElement(mDecoder);
  return true;
}

void
SourceBuffer::DiscardDecoder()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("SourceBuffer(%p)::DiscardDecoder mDecoder=%p", this, mDecoder.get());
  if (mDecoder) {
    mDecoder->SetDiscarded();
  }
  mDecoder = nullptr;
  mDecoderInitialized = false;
  
  mLastParsedTimestamp = UnspecifiedNaN<double>();
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
  MOZ_ASSERT(mUpdating);
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
SourceBuffer::AppendData(const uint8_t* aData, uint32_t aLength, ErrorResult& aRv)
{
  MSE_DEBUG("SourceBuffer(%p)::AppendData(aLength=%u)", this, aLength);
  if (!IsAttached() || mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (mMediaSource->ReadyState() == MediaSourceReadyState::Ended) {
    mMediaSource->SetReadyState(MediaSourceReadyState::Open);
  }
  
  
  StartUpdating();
  
  if (mParser->IsInitSegmentPresent(aData, aLength)) {
    MSE_DEBUG("SourceBuffer(%p)::AppendData: New initialization segment.", this);
    if (mDecoderInitialized) {
      
      DiscardDecoder();
    }

    
    
    if (!mDecoder && !InitNewDecoder()) {
      aRv.Throw(NS_ERROR_FAILURE); 
      return;
    }
    MSE_DEBUG("SourceBuffer(%p)::AppendData: Decoder marked as initialized.", this);
    mDecoderInitialized = true;
  } else if (!mDecoderInitialized) {
    MSE_DEBUG("SourceBuffer(%p)::AppendData: Non-init segment appended during initialization.");
    Optional<MediaSourceEndOfStreamError> decodeError(MediaSourceEndOfStreamError::Decode);
    ErrorResult dummy;
    mMediaSource->EndOfStream(decodeError, dummy);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  double start, end;
  if (mParser->ParseStartAndEndTimestamps(aData, aLength, start, end)) {
    if (start <= mLastParsedTimestamp || mLastParsedTimestamp - start > 0.1) {
      MSE_DEBUG("SourceBuffer(%p)::AppendData: Data (%f, %f) overlaps %f.",
                this, start, end, mLastParsedTimestamp);

      
      
      DiscardDecoder();

      
      
      if (!InitNewDecoder()) {
        aRv.Throw(NS_ERROR_FAILURE); 
        return;
      }
      MSE_DEBUG("SourceBuffer(%p)::AppendData: Decoder marked as initialized.", this);
      mDecoderInitialized = true;
      const nsTArray<uint8_t>& initData = mParser->InitData();
      mDecoder->NotifyDataArrived(reinterpret_cast<const char*>(initData.Elements()),
                                  initData.Length(),
                                  0);
      mDecoder->GetResource()->AppendData(initData.Elements(), initData.Length());
    }
    mLastParsedTimestamp = end;
    MSE_DEBUG("SourceBuffer(%p)::AppendData: Segment start=%f end=%f", this, start, end);
  }
  
  mDecoder->NotifyDataArrived(reinterpret_cast<const char*>(aData),
                              aLength,
                              mDecoder->GetResource()->GetLength());
  mDecoder->GetResource()->AppendData(aData, aLength);

  
  
  
  
  
  
  
  const uint32_t evict_threshold = 75 * (1 << 20);
  bool evicted = mDecoder->GetResource()->EvictData(evict_threshold);
  if (evicted) {
    MSE_DEBUG("SourceBuffer(%p)::AppendData Evict; current buffered start=%f",
              this, GetBufferedStart());

    
    
    mMediaSource->NotifyEvicted(0.0, GetBufferedStart());
  }
  StopUpdating();

  
  
  mMediaSource->GetDecoder()->ScheduleStateMachineThread();

  mMediaSource->GetDecoder()->NotifyGotData();
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
  if (!mDecoder) {
    return;
  }
  double currentTime = mMediaSource->GetDecoder()->GetCurrentTime();
  double evictTime = aEnd;
  const double safety_threshold = 5;
  if (currentTime + safety_threshold >= evictTime) {
    evictTime -= safety_threshold;
  }
  int64_t endOffset = mDecoder->ConvertToByteOffset(evictTime);
  if (endOffset > 0) {
    mDecoder->GetResource()->EvictBefore(endOffset);
  }
  MSE_DEBUG("SourceBuffer(%p)::Evict offset=%lld", this, endOffset);
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(SourceBuffer, DOMEventTargetHelper,
                                   mMediaSource)

NS_IMPL_ADDREF_INHERITED(SourceBuffer, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(SourceBuffer, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(SourceBuffer)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

} 

} 
