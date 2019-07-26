




#include "MediaSourceResource.h"
#include "MediaSourceDecoder.h"

#include "AbstractMediaDecoder.h"
#include "MediaDecoderReader.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/Assertions.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TimeRanges.h"
#include "mozilla/mozalloc.h"
#include "nsISupports.h"
#include "nsIThread.h"
#include "prlog.h"
#include "MediaSource.h"
#include "SubBufferDecoder.h"
#include "SourceBufferResource.h"
#include "VideoUtils.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaSourceLog;
#define MSE_DEBUG(...) PR_LOG(gMediaSourceLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#endif

namespace mozilla {

namespace dom {

class TimeRanges;

} 

class MediaSourceReader : public MediaDecoderReader
{
public:
  MediaSourceReader(MediaSourceDecoder* aDecoder)
    : MediaDecoderReader(aDecoder)
    , mActiveVideoReader(-1)
    , mActiveAudioReader(-1)
  {
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaSourceReader)

  nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE
  {
    
    
    
    return NS_OK;
  }

  bool DecodeAudioData() MOZ_OVERRIDE
  {
    if (mActiveAudioReader == -1) {
      MSE_DEBUG("%p DecodeAudioFrame called with no audio reader", this);
      MOZ_ASSERT(mPendingDecoders.IsEmpty());
      return false;
    }
    bool rv = mAudioReaders[mActiveAudioReader]->DecodeAudioData();

    nsAutoTArray<AudioData*, 10> audio;
    mAudioReaders[mActiveAudioReader]->AudioQueue().GetElementsAfter(-1, &audio);
    for (uint32_t i = 0; i < audio.Length(); ++i) {
      AudioQueue().Push(audio[i]);
    }
    mAudioReaders[mActiveAudioReader]->AudioQueue().Empty();

    return rv;
  }

  bool DecodeVideoFrame(bool& aKeyFrameSkip, int64_t aTimeThreshold) MOZ_OVERRIDE
  {
    if (mActiveVideoReader == -1) {
      MSE_DEBUG("%p DecodeVideoFrame called with no video reader", this);
      MOZ_ASSERT(mPendingDecoders.IsEmpty());
      return false;
    }
    bool rv = mVideoReaders[mActiveVideoReader]->DecodeVideoFrame(aKeyFrameSkip, aTimeThreshold);

    nsAutoTArray<VideoData*, 10> video;
    mVideoReaders[mActiveVideoReader]->VideoQueue().GetElementsAfter(-1, &video);
    for (uint32_t i = 0; i < video.Length(); ++i) {
      VideoQueue().Push(video[i]);
    }
    mVideoReaders[mActiveVideoReader]->VideoQueue().Empty();

    if (rv) {
      return true;
    }

    MSE_DEBUG("%p MSR::DecodeVF %d (%p) returned false (readers=%u)",
              this, mActiveVideoReader, mVideoReaders[mActiveVideoReader], mVideoReaders.Length());
    if (SwitchVideoReaders(aTimeThreshold)) {
      rv = mVideoReaders[mActiveVideoReader]->DecodeVideoFrame(aKeyFrameSkip, aTimeThreshold);

      nsAutoTArray<VideoData*, 10> video;
      mVideoReaders[mActiveVideoReader]->VideoQueue().GetElementsAfter(-1, &video);
      for (uint32_t i = 0; i < video.Length(); ++i) {
        VideoQueue().Push(video[i]);
      }
      mVideoReaders[mActiveVideoReader]->VideoQueue().Empty();
    }
    return rv;
  }

  bool HasVideo() MOZ_OVERRIDE
  {
    return mInfo.HasVideo();
  }

  bool HasAudio() MOZ_OVERRIDE
  {
    return mInfo.HasAudio();
  }

  nsresult ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags) MOZ_OVERRIDE;

  nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
                int64_t aCurrentTime) MOZ_OVERRIDE
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsresult GetBuffered(dom::TimeRanges* aBuffered, int64_t aStartTime) MOZ_OVERRIDE
  {
    for (uint32_t i = 0; i < mVideoReaders.Length(); ++i) {
      nsRefPtr<dom::TimeRanges> r = new dom::TimeRanges();
      mVideoReaders[i]->GetBuffered(r, aStartTime);
      aBuffered->Add(r->GetStartTime(), r->GetEndTime());
    }
    for (uint32_t i = 0; i < mAudioReaders.Length(); ++i) {
      nsRefPtr<dom::TimeRanges> r = new dom::TimeRanges();
      mAudioReaders[i]->GetBuffered(r, aStartTime);
      aBuffered->Add(r->GetStartTime(), r->GetEndTime());
    }
    aBuffered->Normalize();
    return NS_OK;
  }

  already_AddRefed<SubBufferDecoder> CreateSubDecoder(const nsACString& aType,
                                                      MediaSourceDecoder* aParentDecoder);

private:
  bool SwitchVideoReaders(int64_t aTimeThreshold) {
    MOZ_ASSERT(mActiveVideoReader != -1);
    
    
    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

    WaitForPendingDecoders();

    if (mVideoReaders.Length() > uint32_t(mActiveVideoReader + 1)) {
      mActiveVideoReader += 1;
      MSE_DEBUG("%p MSR::DecodeVF switching to %d", this, mActiveVideoReader);

      MOZ_ASSERT(mVideoReaders[mActiveVideoReader]->GetMediaInfo().HasVideo());
      mVideoReaders[mActiveVideoReader]->SetActive();
      mVideoReaders[mActiveVideoReader]->DecodeToTarget(aTimeThreshold);

      return true;
    }
    return false;
  }

  bool EnsureWorkQueueInitialized();
  nsresult EnqueueDecoderInitialization();
  void CallDecoderInitialization();
  void WaitForPendingDecoders();

  nsTArray<nsRefPtr<SubBufferDecoder>> mPendingDecoders;
  nsTArray<nsRefPtr<SubBufferDecoder>> mDecoders;

  nsTArray<MediaDecoderReader*> mVideoReaders;
  nsTArray<MediaDecoderReader*> mAudioReaders;
  int32_t mActiveVideoReader;
  int32_t mActiveAudioReader;

  nsCOMPtr<nsIThread> mWorkQueue;
};

MediaSourceDecoder::MediaSourceDecoder(dom::HTMLMediaElement* aElement)
  : mMediaSource(nullptr)
{
  Init(aElement);
}

MediaDecoder*
MediaSourceDecoder::Clone()
{
  
  return nullptr;
}

MediaDecoderStateMachine*
MediaSourceDecoder::CreateStateMachine()
{
  
  mReader = new MediaSourceReader(this);
  return new MediaDecoderStateMachine(this, mReader);
}

nsresult
MediaSourceDecoder::Load(nsIStreamListener**, MediaDecoder*)
{
  MOZ_ASSERT(!mDecoderStateMachine);
  mDecoderStateMachine = CreateStateMachine();
  if (!mDecoderStateMachine) {
    NS_WARNING("Failed to create state machine!");
    return NS_ERROR_FAILURE;
  }

  return mDecoderStateMachine->Init(nullptr);
}

nsresult
MediaSourceDecoder::GetSeekable(dom::TimeRanges* aSeekable)
{
  double duration = mMediaSource->Duration();
  if (IsNaN(duration)) {
    
  } else if (duration > 0 && mozilla::IsInfinite(duration)) {
    nsRefPtr<dom::TimeRanges> bufferedRanges = new dom::TimeRanges();
    GetBuffered(bufferedRanges);
    aSeekable->Add(bufferedRanges->GetStartTime(), bufferedRanges->GetEndTime());
  } else {
    aSeekable->Add(0, duration);
  }
  return NS_OK;
}


already_AddRefed<MediaResource>
MediaSourceDecoder::CreateResource()
{
  return nsRefPtr<MediaResource>(new MediaSourceResource()).forget();
}

void
MediaSourceDecoder::AttachMediaSource(dom::MediaSource* aMediaSource)
{
  MOZ_ASSERT(!mMediaSource && !mDecoderStateMachine);
  mMediaSource = aMediaSource;
}

void
MediaSourceDecoder::DetachMediaSource()
{
  mMediaSource = nullptr;
}

already_AddRefed<SubBufferDecoder>
MediaSourceDecoder::CreateSubDecoder(const nsACString& aType)
{
  return mReader->CreateSubDecoder(aType, this);
}

bool
MediaSourceReader::EnsureWorkQueueInitialized()
{
  
  MOZ_ASSERT(NS_IsMainThread());
  if (!mWorkQueue &&
      NS_FAILED(NS_NewNamedThread("MediaSource",
                                  getter_AddRefs(mWorkQueue),
                                  nullptr,
                                  MEDIA_THREAD_STACK_SIZE))) {
    return false;
  }
  return true;
}

nsresult
MediaSourceReader::EnqueueDecoderInitialization()
{
  if (!EnsureWorkQueueInitialized()) {
    return NS_ERROR_FAILURE;
  }
  return mWorkQueue->Dispatch(NS_NewRunnableMethod(this, &MediaSourceReader::CallDecoderInitialization), NS_DISPATCH_NORMAL);
}

void
MediaSourceReader::CallDecoderInitialization()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  for (uint32_t i = 0; i < mPendingDecoders.Length(); ++i) {
    nsRefPtr<SubBufferDecoder> decoder = mPendingDecoders[i];
    MediaDecoderReader* reader = decoder->GetReader();
    MSE_DEBUG("%p: Initializating subdecoder %p reader %p", this, decoder.get(), reader);

    reader->SetActive();
    MediaInfo mi;
    nsAutoPtr<MetadataTags> tags; 
    nsresult rv;
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      rv = reader->ReadMetadata(&mi, getter_Transfers(tags));
    }
    reader->SetIdle();
    if (NS_FAILED(rv)) {
      
      MSE_DEBUG("%p: Reader %p failed to initialize, rv=%x", this, reader, rv);
      continue;
    }

    bool active = false;
    if (mi.HasVideo()) {
      MSE_DEBUG("%p: Reader %p has video track", this, reader);
      mVideoReaders.AppendElement(reader);
      active = true;
    }
    if (mi.HasAudio()) {
      MSE_DEBUG("%p: Reader %p has audio track", this, reader);
      mAudioReaders.AppendElement(reader);
      active = true;
    }

    if (active) {
      mDecoders.AppendElement(decoder);
    } else {
      MSE_DEBUG("%p: Reader %p not activated", this, reader);
    }
  }
  mPendingDecoders.Clear();
  mon.NotifyAll();
}

void
MediaSourceReader::WaitForPendingDecoders()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  while (!mPendingDecoders.IsEmpty()) {
    mon.Wait();
  }
}

already_AddRefed<SubBufferDecoder>
MediaSourceReader::CreateSubDecoder(const nsACString& aType, MediaSourceDecoder* aParentDecoder)
{
  nsRefPtr<SubBufferDecoder> decoder =
    new SubBufferDecoder(new SourceBufferResource(nullptr, aType), aParentDecoder);
  nsAutoPtr<MediaDecoderReader> reader(DecoderTraits::CreateReader(aType, decoder));
  if (!reader) {
    return nullptr;
  }
  reader->Init(nullptr);
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MSE_DEBUG("Registered subdecoder %p subreader %p", decoder.get(), reader.get());
  decoder->SetReader(reader.forget());
  mPendingDecoders.AppendElement(decoder);
  EnqueueDecoderInitialization();
  return decoder.forget();
}

nsresult
MediaSourceReader::ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags)
{
  mDecoder->SetMediaSeekable(true);
  mDecoder->SetTransportSeekable(false);

  WaitForPendingDecoders();

  
  
  
  
  
  
  
  int64_t maxDuration = -1;
  for (uint32_t i = 0; i < mDecoders.Length(); ++i) {
    MediaDecoderReader* reader = mDecoders[i]->GetReader();

    reader->SetActive(); 

    MediaInfo mi = reader->GetMediaInfo();

    if (mi.HasVideo() && !mInfo.HasVideo()) {
      MOZ_ASSERT(mActiveVideoReader == -1);
      mActiveVideoReader = i;
      mInfo.mVideo = mi.mVideo;
      maxDuration = std::max(maxDuration, mDecoders[i]->GetMediaDuration());
    }
    if (mi.HasAudio() && !mInfo.HasAudio()) {
      MOZ_ASSERT(mActiveAudioReader == -1);
      mActiveAudioReader = i;
      mInfo.mAudio = mi.mAudio;
      maxDuration = std::max(maxDuration, mDecoders[i]->GetMediaDuration());
    }
  }
  *aInfo = mInfo;

  if (maxDuration != -1) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(maxDuration);
  }

  return NS_OK;
}

} 
