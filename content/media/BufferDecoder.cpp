





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
BufferDecoder::NotifyBytesConsumed(int64_t aBytes)
{
  
}

void
BufferDecoder::NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded)
{
  
}

int64_t
BufferDecoder::GetEndMediaTime() const
{
  
  return -1;
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
BufferDecoder::UpdateMediaDuration(int64_t aDuration)
{
  
}

void
BufferDecoder::SetMediaSeekable(bool aMediaSeekable)
{
  
}

void
BufferDecoder::SetTransportSeekable(bool aTransportSeekable)
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
BufferDecoder::MetadataLoaded(int aChannels, int aRate, bool aHasAudio, bool aHasVideo, MetadataTags* aTags)
{
  
}

void
BufferDecoder::QueueMetadata(int64_t aTime, int aChannels, int aRate, bool aHasAudio, bool aHasVideo, MetadataTags* aTags)
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

MediaDecoderOwner*
BufferDecoder::GetOwner()
{
  
  return nullptr;
}

} 
