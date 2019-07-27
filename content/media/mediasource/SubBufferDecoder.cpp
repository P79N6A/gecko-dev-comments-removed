




#include "SubBufferDecoder.h"
#include "MediaSourceDecoder.h"
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
  MSE_DEBUG("SubBufferDecoder(%p)::UpdateEstimatedMediaDuration(aDuration=%lld)", this, aDuration);
}

void
SubBufferDecoder::SetMediaSeekable(bool aMediaSeekable)
{
  MSE_DEBUG("SubBufferDecoder(%p)::SetMediaSeekable(aMediaSeekable=%d)", this, aMediaSeekable);
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

void
SubBufferDecoder::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  mReader->NotifyDataArrived(aBuffer, aLength, aOffset);

  
  
  
  
  mParentDecoder->NotifyDataArrived(nullptr, 0, 0);
}

nsresult
SubBufferDecoder::GetBuffered(dom::TimeRanges* aBuffered)
{
  
  return mReader->GetBuffered(aBuffered, 0);
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

bool
SubBufferDecoder::ContainsTime(double aTime)
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
