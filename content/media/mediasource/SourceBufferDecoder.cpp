




#include "SourceBufferDecoder.h"
#include "prlog.h"
#include "AbstractMediaDecoder.h"
#include "MediaDecoderReader.h"
#include "mozilla/dom/TimeRanges.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaSourceLog();
extern PRLogModuleInfo* GetMediaSourceAPILog();

#define MSE_DEBUG(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#define MSE_API(...) PR_LOG(GetMediaSourceAPILog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#define MSE_API(...)
#endif

namespace mozilla {

class ReentrantMonitor;

namespace layers {

class ImageContainer;

} 

NS_IMPL_ISUPPORTS0(SourceBufferDecoder)

SourceBufferDecoder::SourceBufferDecoder(MediaResource* aResource,
                                         AbstractMediaDecoder* aParentDecoder)
  : mResource(aResource)
  , mParentDecoder(aParentDecoder)
  , mReader(nullptr)
  , mMediaDuration(-1)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(SourceBufferDecoder);
}

SourceBufferDecoder::~SourceBufferDecoder()
{
  MOZ_COUNT_DTOR(SourceBufferDecoder);
}

bool
SourceBufferDecoder::IsShutdown() const
{
  
  MSE_DEBUG("SourceBufferDecoder(%p)::IsShutdown UNIMPLEMENTED", this);
  return false;
}

void
SourceBufferDecoder::NotifyBytesConsumed(int64_t aBytes, int64_t aOffset)
{
  MSE_DEBUG("SourceBufferDecoder(%p)::NotifyBytesConsumed UNIMPLEMENTED", this);
}

int64_t
SourceBufferDecoder::GetMediaDuration()
{
  return mMediaDuration;
}

VideoFrameContainer*
SourceBufferDecoder::GetVideoFrameContainer()
{
  MSE_DEBUG("SourceBufferDecoder(%p)::GetVideoFrameContainer UNIMPLEMENTED", this);
  return nullptr;
}

bool
SourceBufferDecoder::IsTransportSeekable()
{
  MSE_DEBUG("SourceBufferDecoder(%p)::IsTransportSeekable UNIMPLEMENTED", this);
  return false;
}

bool
SourceBufferDecoder::IsMediaSeekable()
{
  MSE_DEBUG("SourceBufferDecoder(%p)::IsMediaSeekable UNIMPLEMENTED", this);
  return false;
}

void
SourceBufferDecoder::MetadataLoaded(MediaInfo* aInfo, MetadataTags* aTags)
{
  MSE_DEBUG("SourceBufferDecoder(%p)::MetadataLoaded UNIMPLEMENTED", this);
}

void
SourceBufferDecoder::QueueMetadata(int64_t aTime, MediaInfo* aInfo, MetadataTags* aTags)
{
  MSE_DEBUG("SourceBufferDecoder(%p)::QueueMetadata UNIMPLEMENTED", this);
}

void
SourceBufferDecoder::RemoveMediaTracks()
{
  MSE_DEBUG("SourceBufferDecoder(%p)::RemoveMediaTracks UNIMPLEMENTED", this);
}

void
SourceBufferDecoder::SetMediaEndTime(int64_t aTime)
{
  MSE_DEBUG("SourceBufferDecoder(%p)::SetMediaEndTime UNIMPLEMENTED", this);
}

void
SourceBufferDecoder::UpdatePlaybackPosition(int64_t aTime)
{
  MSE_DEBUG("SourceBufferDecoder(%p)::UpdatePlaybackPosition UNIMPLEMENTED", this);
}

void
SourceBufferDecoder::OnReadMetadataCompleted()
{
  MSE_DEBUG("SourceBufferDecoder(%p)::OnReadMetadataCompleted UNIMPLEMENTED", this);
}

void
SourceBufferDecoder::NotifyWaitingForResourcesStatusChanged()
{
  MSE_DEBUG("SourceBufferDecoder(%p)::NotifyWaitingForResourcesStatusChanged UNIMPLEMENTED", this);
}

ReentrantMonitor&
SourceBufferDecoder::GetReentrantMonitor()
{
  return mParentDecoder->GetReentrantMonitor();
}

bool
SourceBufferDecoder::OnStateMachineThread() const
{
  return mParentDecoder->OnStateMachineThread();
}

bool
SourceBufferDecoder::OnDecodeThread() const
{
  
  if (mTaskQueue) {
    return mTaskQueue->IsCurrentThreadIn();
  }
  return mParentDecoder->OnDecodeThread();
}

SourceBufferResource*
SourceBufferDecoder::GetResource() const
{
  return static_cast<SourceBufferResource*>(mResource.get());
}

void
SourceBufferDecoder::NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded)
{
  return mParentDecoder->NotifyDecodedFrames(aParsed, aDecoded);
}

void
SourceBufferDecoder::SetMediaDuration(int64_t aDuration)
{
  mMediaDuration = aDuration;
}

void
SourceBufferDecoder::UpdateEstimatedMediaDuration(int64_t aDuration)
{
  MSE_DEBUG("SourceBufferDecoder(%p)::UpdateEstimatedMediaDuration UNIMPLEMENTED", this);
}

void
SourceBufferDecoder::SetMediaSeekable(bool aMediaSeekable)
{
  MSE_DEBUG("SourceBufferDecoder(%p)::SetMediaSeekable UNIMPLEMENTED", this);
}

layers::ImageContainer*
SourceBufferDecoder::GetImageContainer()
{
  return mParentDecoder->GetImageContainer();
}

MediaDecoderOwner*
SourceBufferDecoder::GetOwner()
{
  return mParentDecoder->GetOwner();
}

void
SourceBufferDecoder::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  mReader->NotifyDataArrived(aBuffer, aLength, aOffset);

  
  
  
  
  mParentDecoder->NotifyDataArrived(nullptr, 0, 0);
}

nsresult
SourceBufferDecoder::GetBuffered(dom::TimeRanges* aBuffered)
{
  
  return mReader->GetBuffered(aBuffered, 0);
}

int64_t
SourceBufferDecoder::ConvertToByteOffset(double aTime)
{
  int64_t readerOffset = mReader->GetEvictionOffset(aTime);
  if (readerOffset >= 0) {
    return readerOffset;
  }

  
  
  
  
  if (mMediaDuration <= 0) {
    return -1;
  }
  int64_t length = GetResource()->GetLength();
  MOZ_ASSERT(length > 0);
  int64_t offset = (aTime / (double(mMediaDuration) / USECS_PER_S)) * length;
  return offset;
}

bool
SourceBufferDecoder::ContainsTime(double aTime)
{
  ErrorResult dummy;
  nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
  nsresult rv = GetBuffered(ranges);
  if (NS_FAILED(rv) || ranges->Length() == 0) {
    return false;
  }
  return ranges->Find(aTime) != dom::TimeRanges::NoIndex;
}

} 
