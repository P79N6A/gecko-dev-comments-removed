





#include "BufferDecoder.h"

#include "nsISupports.h"
#include "MediaResource.h"

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#endif

NS_IMPL_ISUPPORTS0(BufferDecoder)

BufferDecoder::BufferDecoder(MediaResource* aResource)
  : mReentrantMonitor("BufferDecoder")
  , mResource(aResource)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(BufferDecoder);
#ifdef PR_LOGGING
  if (!gMediaDecoderLog) {
    gMediaDecoderLog = PR_NewLogModule("MediaDecoder");
  }
#endif
}

BufferDecoder::~BufferDecoder()
{
  
  MOZ_COUNT_DTOR(BufferDecoder);
}

void
BufferDecoder::BeginDecoding(nsIThread* aDecodeThread)
{
  MOZ_ASSERT(!mDecodeThread && aDecodeThread);
  mDecodeThread = aDecodeThread;
}

ReentrantMonitor&
BufferDecoder::GetReentrantMonitor()
{
  return mReentrantMonitor;
}

bool
BufferDecoder::IsShutdown() const
{
  
  return false;
}

bool
BufferDecoder::OnStateMachineThread() const
{
  
  return true;
}

bool
BufferDecoder::OnDecodeThread() const
{
  MOZ_ASSERT(mDecodeThread, "Forgot to call BeginDecoding?");
  return IsCurrentThread(mDecodeThread);
}

MediaResource*
BufferDecoder::GetResource() const
{
  return mResource;
}

void
BufferDecoder::NotifyBytesConsumed(int64_t aBytes, int64_t aOffset)
{
  
}

void
BufferDecoder::NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded)
{
  
}

int64_t
BufferDecoder::GetMediaDuration()
{
  
  return -1;
}

void
BufferDecoder::SetMediaDuration(int64_t aDuration)
{
  
}

void
BufferDecoder::UpdateEstimatedMediaDuration(int64_t aDuration)
{
  
}

void
BufferDecoder::SetMediaSeekable(bool aMediaSeekable)
{
  
}

VideoFrameContainer*
BufferDecoder::GetVideoFrameContainer()
{
  
  return nullptr;
}

layers::ImageContainer*
BufferDecoder::GetImageContainer()
{
  
  return nullptr;
}

bool
BufferDecoder::IsTransportSeekable()
{
  return false;
}

bool
BufferDecoder::IsMediaSeekable()
{
  return false;
}

void
BufferDecoder::MetadataLoaded(MediaInfo* aInfo, MetadataTags* aTags)
{
  
}

void
BufferDecoder::QueueMetadata(int64_t aTime, MediaInfo* aInfo, MetadataTags* aTags)
{
  
}

void
BufferDecoder::RemoveMediaTracks()
{
  
}

void
BufferDecoder::SetMediaEndTime(int64_t aTime)
{
  
}

void
BufferDecoder::UpdatePlaybackPosition(int64_t aTime)
{
  
}

void
BufferDecoder::OnReadMetadataCompleted()
{
  
}

void
BufferDecoder::NotifyWaitingForResourcesStatusChanged()
{
  
}

void
BufferDecoder::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  
}

MediaDecoderOwner*
BufferDecoder::GetOwner()
{
  
  return nullptr;
}

} 
