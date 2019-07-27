





#include "MediaSourceUtils.h"
#include "SourceBufferDecoder.h"
#include "mozilla/Logging.h"
#include "AbstractMediaDecoder.h"
#include "MediaDecoderReader.h"

extern PRLogModuleInfo* GetMediaSourceLog();

#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

#define MSE_DEBUG(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Debug, ("SourceBufferDecoder(%p:%s)::%s: " arg, this, mResource->GetContentType().get(), __func__, ##__VA_ARGS__))

namespace mozilla {

class ReentrantMonitor;

namespace layers {

class ImageContainer;

} 

NS_IMPL_ISUPPORTS0(SourceBufferDecoder)

SourceBufferDecoder::SourceBufferDecoder(MediaResource* aResource,
                                         AbstractMediaDecoder* aParentDecoder,
                                         int64_t aTimestampOffset)
  : mResource(aResource)
  , mParentDecoder(aParentDecoder)
  , mReader(nullptr)
  , mTimestampOffset(aTimestampOffset)
  , mRealMediaDuration(0)
  , mTrimmedOffset(-1)
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
  
  MSE_DEBUG("UNIMPLEMENTED");
  return false;
}

void
SourceBufferDecoder::NotifyBytesConsumed(int64_t aBytes, int64_t aOffset)
{
  MSE_DEBUG("UNIMPLEMENTED");
}

VideoFrameContainer*
SourceBufferDecoder::GetVideoFrameContainer()
{
  MSE_DEBUG("UNIMPLEMENTED");
  return nullptr;
}

bool
SourceBufferDecoder::IsTransportSeekable()
{
  MSE_DEBUG("UNIMPLEMENTED");
  return false;
}

bool
SourceBufferDecoder::IsMediaSeekable()
{
  MSE_DEBUG("UNIMPLEMENTED");
  return false;
}

void
SourceBufferDecoder::MetadataLoaded(nsAutoPtr<MediaInfo> aInfo,
                                    nsAutoPtr<MetadataTags> aTags,
                                    MediaDecoderEventVisibility aEventVisibility)
{
  MSE_DEBUG("UNIMPLEMENTED");
}

void
SourceBufferDecoder::FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                      MediaDecoderEventVisibility aEventVisibility)
{
  MSE_DEBUG("UNIMPLEMENTED");
}

void
SourceBufferDecoder::QueueMetadata(int64_t aTime,
                                   nsAutoPtr<MediaInfo> aInfo,
                                   nsAutoPtr<MetadataTags> aTags)
{
  MSE_DEBUG("UNIMPLEMENTED");
}

void
SourceBufferDecoder::RemoveMediaTracks()
{
  MSE_DEBUG("UNIMPLEMENTED");
}

bool
SourceBufferDecoder::HasInitializationData()
{
  return true;
}

void
SourceBufferDecoder::OnReadMetadataCompleted()
{
  MSE_DEBUG("UNIMPLEMENTED");
}

void
SourceBufferDecoder::NotifyWaitingForResourcesStatusChanged()
{
  MSE_DEBUG("UNIMPLEMENTED");
}

ReentrantMonitor&
SourceBufferDecoder::GetReentrantMonitor()
{
  return mParentDecoder->GetReentrantMonitor();
}

bool
SourceBufferDecoder::OnStateMachineTaskQueue() const
{
  return mParentDecoder->OnStateMachineTaskQueue();
}

bool
SourceBufferDecoder::OnDecodeTaskQueue() const
{
  
  if (mTaskQueue) {
    return mTaskQueue->IsCurrentThreadIn();
  }
  return mParentDecoder->OnDecodeTaskQueue();
}

SourceBufferResource*
SourceBufferDecoder::GetResource() const
{
  return static_cast<SourceBufferResource*>(mResource.get());
}

void
SourceBufferDecoder::NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded,
                                         uint32_t aDropped)
{
  return mParentDecoder->NotifyDecodedFrames(aParsed, aDecoded, aDropped);
}

void
SourceBufferDecoder::SetRealMediaDuration(int64_t aDuration)
{
  mRealMediaDuration = aDuration;
}

void
SourceBufferDecoder::Trim(int64_t aDuration)
{
  mTrimmedOffset = (double)aDuration / USECS_PER_S;
}

void
SourceBufferDecoder::SetMediaSeekable(bool aMediaSeekable)
{
  MSE_DEBUG("UNIMPLEMENTED");
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

media::TimeIntervals
SourceBufferDecoder::GetBuffered()
{
  media::TimeIntervals buffered = mReader->GetBuffered();
  if (buffered.IsInvalid()) {
    return buffered;
  }

  
  buffered.Shift(media::TimeUnit::FromMicroseconds(mTimestampOffset));

  if (!WasTrimmed()) {
    return buffered;
  }
  media::TimeInterval filter(media::TimeUnit::FromSeconds(0),
                             media::TimeUnit::FromSeconds(mTrimmedOffset));
  return buffered.Intersection(filter);
}

int64_t
SourceBufferDecoder::ConvertToByteOffset(double aTime)
{
  int64_t readerOffset =
    mReader->GetEvictionOffset(aTime - double(mTimestampOffset) / USECS_PER_S);
  if (readerOffset >= 0) {
    return readerOffset;
  }

  
  
  
  
  if (mRealMediaDuration <= 0) {
    return -1;
  }
  int64_t length = GetResource()->GetLength();
  MOZ_ASSERT(length > 0);
  int64_t offset =
    ((aTime - double(mTimestampOffset) / USECS_PER_S) /
      (double(mRealMediaDuration) / USECS_PER_S)) * length;
  return offset;
}

#undef MSE_DEBUG
} 
