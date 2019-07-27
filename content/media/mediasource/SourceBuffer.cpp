




#include "SourceBuffer.h"

#include "AsyncEventRunner.h"
#include "DecoderTraits.h"
#include "MediaDecoder.h"
#include "MediaSourceDecoder.h"
#include "SourceBufferResource.h"
#include "mozilla/Endian.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/dom/MediaSourceBinding.h"
#include "mozilla/dom/TimeRanges.h"
#include "nsError.h"
#include "nsIEventTarget.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"
#include "prlog.h"
#include "SubBufferDecoder.h"

struct JSContext;
class JSObject;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaSourceLog;
#define MSE_DEBUG(...) PR_LOG(gMediaSourceLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#endif

namespace mozilla {

class MediaResource;
class ReentrantMonitor;

namespace layers {

class ImageContainer;

} 

ReentrantMonitor&
SubBufferDecoder::GetReentrantMonitor()
{
  return mParentDecoder->GetReentrantMonitor();
}

bool
SubBufferDecoder::OnStateMachineThread() const
{
  return mParentDecoder->OnStateMachineThread();
}

bool
SubBufferDecoder::OnDecodeThread() const
{
  return mParentDecoder->OnDecodeThread();
}

SourceBufferResource*
SubBufferDecoder::GetResource() const
{
  return static_cast<SourceBufferResource*>(mResource.get());
}

void
SubBufferDecoder::NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded)
{
  return mParentDecoder->NotifyDecodedFrames(aParsed, aDecoded);
}

void
SubBufferDecoder::SetMediaDuration(int64_t aDuration)
{
  mMediaDuration = aDuration;
}

void
SubBufferDecoder::UpdateEstimatedMediaDuration(int64_t aDuration)
{
  
}

void
SubBufferDecoder::SetMediaSeekable(bool aMediaSeekable)
{
  
}

layers::ImageContainer*
SubBufferDecoder::GetImageContainer()
{
  return mParentDecoder->GetImageContainer();
}

MediaDecoderOwner*
SubBufferDecoder::GetOwner()
{
  return mParentDecoder->GetOwner();
}

int64_t
SubBufferDecoder::ConvertToByteOffset(double aTime)
{
  
  
  
  
  if (mMediaDuration == -1) {
    return -1;
  }
  int64_t length = GetResource()->GetLength();
  MOZ_ASSERT(length > 0);
  int64_t offset = (aTime / (double(mMediaDuration) / USECS_PER_S)) * length;
  return offset;
}

class ContainerParser {
public:
  virtual ~ContainerParser() {}

  virtual bool IsInitSegmentPresent(const uint8_t* aData, uint32_t aLength)
  {
    return false;
  }

  static ContainerParser* CreateForMIMEType(const nsACString& aType);
};

class WebMContainerParser : public ContainerParser {
public:
  bool IsInitSegmentPresent(const uint8_t* aData, uint32_t aLength)
  {
    
    
    
    
    
    
    
    
    
    
    if (aLength >= 4 &&
        aData[0] == 0x1a && aData[1] == 0x45 && aData[2] == 0xdf && aData[3] == 0xa3) {
      return true;
    }
    return false;
  }
};

class MP4ContainerParser : public ContainerParser {
public:
  bool IsInitSegmentPresent(const uint8_t* aData, uint32_t aLength)
  {
    
    
    

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
  if (!IsAttached()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }
  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  if (mDecoder) {
    mDecoder->GetBuffered(ranges);
  }
  ranges->Normalize();
  return ranges.forget();
}

void
SourceBuffer::SetAppendWindowStart(double aAppendWindowStart, ErrorResult& aRv)
{
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
  aData.ComputeLengthAndData();

  AppendData(aData.Data(), aData.Length(), aRv);
}

void
SourceBuffer::AppendBuffer(const ArrayBufferView& aData, ErrorResult& aRv)
{
  aData.ComputeLengthAndData();

  AppendData(aData.Data(), aData.Length(), aRv);
}

void
SourceBuffer::Abort(ErrorResult& aRv)
{
  MSE_DEBUG("%p Abort()", this);
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

  MSE_DEBUG("%p Abort: Discarding decoder.", this);
  DiscardDecoder();
}

void
SourceBuffer::Remove(double aStart, double aEnd, ErrorResult& aRv)
{
  MSE_DEBUG("%p Remove(Start=%f End=%f)", this, aStart, aEnd);
  if (!IsAttached() || mUpdating ||
      mMediaSource->ReadyState() != MediaSourceReadyState::Open) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (aStart < 0 || aStart > mMediaSource->Duration() ||
      aEnd <= aStart) {
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }
  StartUpdating();
  
  StopUpdating();
}

void
SourceBuffer::Detach()
{
  Ended();
  DiscardDecoder();
  mMediaSource = nullptr;
}

void
SourceBuffer::Ended()
{
  if (mDecoder) {
    mDecoder->GetResource()->Ended();
  }
}

SourceBuffer::SourceBuffer(MediaSource* aMediaSource, const nsACString& aType)
  : DOMEventTargetHelper(aMediaSource->GetParentObject())
  , mMediaSource(aMediaSource)
  , mType(aType)
  , mAppendWindowStart(0)
  , mAppendWindowEnd(PositiveInfinity<double>())
  , mTimestampOffset(0)
  , mAppendMode(SourceBufferAppendMode::Segments)
  , mUpdating(false)
  , mDecoderInitialized(false)
{
  MOZ_ASSERT(aMediaSource);
  mParser = ContainerParser::CreateForMIMEType(aType);
  MSE_DEBUG("%p SourceBuffer: Creating initial decoder.", this);
  InitNewDecoder();
}

already_AddRefed<SourceBuffer>
SourceBuffer::Create(MediaSource* aMediaSource, const nsACString& aType)
{
  nsRefPtr<SourceBuffer> sourceBuffer = new SourceBuffer(aMediaSource, aType);
  return sourceBuffer.forget();
}

SourceBuffer::~SourceBuffer()
{
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
  MSE_DEBUG("%p Dispatching event %s to SourceBuffer", this, aName);
  DispatchTrustedEvent(NS_ConvertUTF8toUTF16(aName));
}

void
SourceBuffer::QueueAsyncSimpleEvent(const char* aName)
{
  MSE_DEBUG("%p Queuing event %s to SourceBuffer", this, aName);
  nsCOMPtr<nsIRunnable> event = new AsyncEventRunner<SourceBuffer>(this, aName);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

bool
SourceBuffer::InitNewDecoder()
{
  MOZ_ASSERT(!mDecoder);
  MediaSourceDecoder* parentDecoder = mMediaSource->GetDecoder();
  nsRefPtr<SubBufferDecoder> decoder = parentDecoder->CreateSubDecoder(mType);
  if (!decoder) {
    return false;
  }
  mDecoder = decoder;
  mDecoderInitialized = false;
  return true;
}

void
SourceBuffer::DiscardDecoder()
{
  if (mDecoder) {
    mDecoder->SetDiscarded();
  }
  mDecoder = nullptr;
  mDecoderInitialized = false;
}

void
SourceBuffer::StartUpdating()
{
  MOZ_ASSERT(!mUpdating);
  mUpdating = true;
  QueueAsyncSimpleEvent("updatestart");
}

void
SourceBuffer::StopUpdating()
{
  MOZ_ASSERT(mUpdating);
  mUpdating = false;
  QueueAsyncSimpleEvent("update");
  QueueAsyncSimpleEvent("updateend");
}

void
SourceBuffer::AbortUpdating()
{
  MOZ_ASSERT(mUpdating);
  mUpdating = false;
  QueueAsyncSimpleEvent("abort");
  QueueAsyncSimpleEvent("updateend");
}

void
SourceBuffer::AppendData(const uint8_t* aData, uint32_t aLength, ErrorResult& aRv)
{
  MSE_DEBUG("%p AppendBuffer(Data=%u bytes)", this, aLength);
  if (!IsAttached() || mUpdating) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  if (mMediaSource->ReadyState() == MediaSourceReadyState::Ended) {
    mMediaSource->SetReadyState(MediaSourceReadyState::Open);
  }
  
  
  StartUpdating();
  
  if (mParser->IsInitSegmentPresent(aData, aLength)) {
    MSE_DEBUG("%p AppendBuffer: New initialization segment.", this);
    if (mDecoderInitialized) {
      
      DiscardDecoder();
    }

    
    
    if (!mDecoder && !InitNewDecoder()) {
      aRv.Throw(NS_ERROR_FAILURE); 
      return;
    }
    MSE_DEBUG("%p AppendBuffer: Decoder marked as initialized.", this);
    mDecoderInitialized = true;
  } else if (!mDecoderInitialized) {
    MSE_DEBUG("%p AppendBuffer: Non-initialization segment appended during initialization.");
    Optional<MediaSourceEndOfStreamError> decodeError(MediaSourceEndOfStreamError::Decode);
    ErrorResult dummy;
    mMediaSource->EndOfStream(decodeError, dummy);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  
  mDecoder->NotifyDataArrived(reinterpret_cast<const char*>(aData),
                              aLength,
                              mDecoder->GetResource()->GetLength());
  mDecoder->GetResource()->AppendData(aData, aLength);

  
  
  
  
  
  const int evict_threshold = 1000000;
  bool evicted = mDecoder->GetResource()->EvictData(evict_threshold);
  if (evicted) {
    double start = 0.0;
    double end = 0.0;
    GetBufferedStartEndTime(&start, &end);

    
    
    mMediaSource->NotifyEvicted(0.0, start);
  }
  StopUpdating();

  
  
  mMediaSource->GetDecoder()->ScheduleStateMachineThread();

  mMediaSource->NotifyGotData();
}

void
SourceBuffer::GetBufferedStartEndTime(double* aStart, double* aEnd)
{
  ErrorResult dummy;
  nsRefPtr<TimeRanges> ranges = GetBuffered(dummy);
  if (!ranges || ranges->Length() == 0) {
    *aStart = *aEnd = 0.0;
    return;
  }
  *aStart = ranges->Start(0, dummy);
  *aEnd = ranges->End(ranges->Length() - 1, dummy);
}

void
SourceBuffer::Evict(double aStart, double aEnd)
{
  if (!mDecoder) {
    return;
  }
  
  int64_t end = mDecoder->ConvertToByteOffset(aEnd);
  if (end > 0) {
    mDecoder->GetResource()->EvictBefore(end);
  } else {
    NS_WARNING("SourceBuffer::Evict failed");
  }
}

bool
SourceBuffer::ContainsTime(double aTime)
{
  ErrorResult dummy;
  nsRefPtr<TimeRanges> ranges = GetBuffered(dummy);
  if (!ranges || ranges->Length() == 0) {
    return false;
  }
  for (uint32_t i = 0; i < ranges->Length(); ++i) {
    if (aTime >= ranges->Start(i, dummy) &&
        aTime <= ranges->End(i, dummy)) {
      return true;
    }
  }
  return false;
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(SourceBuffer, DOMEventTargetHelper,
                                   mMediaSource)

NS_IMPL_ADDREF_INHERITED(SourceBuffer, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(SourceBuffer, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(SourceBuffer)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

} 

} 
