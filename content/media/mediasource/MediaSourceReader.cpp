




#include "MediaSourceReader.h"

#include "prlog.h"
#include "mozilla/dom/TimeRanges.h"
#include "DecoderTraits.h"
#include "MediaDataDecodedListener.h"
#include "MediaDecoderOwner.h"
#include "MediaSource.h"
#include "MediaSourceDecoder.h"
#include "MediaSourceUtils.h"
#include "SourceBufferDecoder.h"

#ifdef MOZ_FMP4
#include "MP4Decoder.h"
#include "MP4Reader.h"
#endif

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

MediaSourceReader::MediaSourceReader(MediaSourceDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mTimeThreshold(-1)
  , mDropAudioBeforeThreshold(false)
  , mDropVideoBeforeThreshold(false)
  , mEnded(false)
{
}

bool
MediaSourceReader::IsWaitingMediaResources()
{
  return mDecoders.IsEmpty() && mPendingDecoders.IsEmpty();
}

void
MediaSourceReader::RequestAudioData()
{
  if (!mAudioReader) {
    MSE_DEBUG("MediaSourceReader(%p)::RequestAudioData called with no audio reader", this);
    MOZ_ASSERT(mPendingDecoders.IsEmpty());
    GetCallback()->OnDecodeError();
    return;
  }
  SwitchReaders(SWITCH_OPTIONAL);
  mAudioReader->RequestAudioData();
}

void
MediaSourceReader::OnAudioDecoded(AudioData* aSample)
{
  if (mDropAudioBeforeThreshold) {
    if (aSample->mTime < mTimeThreshold) {
      MSE_DEBUG("MediaSourceReader(%p)::OnAudioDecoded mTime=%lld < mTimeThreshold=%lld",
                this, aSample->mTime, mTimeThreshold);
      delete aSample;
      mAudioReader->RequestAudioData();
      return;
    }
    mDropAudioBeforeThreshold = false;
  }
  GetCallback()->OnAudioDecoded(aSample);
}

void
MediaSourceReader::OnAudioEOS()
{
  MSE_DEBUG("MediaSourceReader(%p)::OnAudioEOS reader=%p (readers=%u)",
            this, mAudioReader.get(), mDecoders.Length());
  if (SwitchReaders(SWITCH_FORCED)) {
    
    RequestAudioData();
  } else {
    
    MSE_DEBUG("MediaSourceReader(%p)::OnAudioEOS reader=%p EOS (readers=%u)",
              this, mAudioReader.get(), mDecoders.Length());
    GetCallback()->OnAudioEOS();
  }
}

void
MediaSourceReader::RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold)
{
  if (!mVideoReader) {
    MSE_DEBUG("MediaSourceReader(%p)::RequestVideoData called with no video reader", this);
    MOZ_ASSERT(mPendingDecoders.IsEmpty());
    GetCallback()->OnDecodeError();
    return;
  }
  mTimeThreshold = aTimeThreshold;
  SwitchReaders(SWITCH_OPTIONAL);
  mVideoReader->RequestVideoData(aSkipToNextKeyframe, aTimeThreshold);
}

void
MediaSourceReader::OnVideoDecoded(VideoData* aSample)
{
  if (mDropVideoBeforeThreshold) {
    if (aSample->mTime < mTimeThreshold) {
      MSE_DEBUG("MediaSourceReader(%p)::OnVideoDecoded mTime=%lld < mTimeThreshold=%lld",
                this, aSample->mTime, mTimeThreshold);
      delete aSample;
      mVideoReader->RequestVideoData(false, mTimeThreshold);
      return;
    }
    mDropVideoBeforeThreshold = false;
  }
  GetCallback()->OnVideoDecoded(aSample);
}

void
MediaSourceReader::OnVideoEOS()
{
  
  MSE_DEBUG("MediaSourceReader(%p)::OnVideoEOS reader=%p (readers=%u)",
            this, mVideoReader.get(), mDecoders.Length());
  if (SwitchReaders(SWITCH_FORCED)) {
    
    RequestVideoData(false, mTimeThreshold);
  } else if (IsEnded()) {
    
    MSE_DEBUG("MediaSourceReader(%p)::OnVideoEOS reader=%p EOS (readers=%u)",
              this, mVideoReader.get(), mDecoders.Length());
    GetCallback()->OnVideoEOS();
  }
}

void
MediaSourceReader::OnDecodeError()
{
  GetCallback()->OnDecodeError();
}

void
MediaSourceReader::Shutdown()
{
  MediaDecoderReader::Shutdown();
  for (uint32_t i = 0; i < mDecoders.Length(); ++i) {
    mDecoders[i]->GetReader()->Shutdown();
  }
}

void
MediaSourceReader::BreakCycles()
{
  MediaDecoderReader::BreakCycles();
  for (uint32_t i = 0; i < mDecoders.Length(); ++i) {
    mDecoders[i]->GetReader()->BreakCycles();
  }
}

bool
MediaSourceReader::SwitchAudioReader(MediaDecoderReader* aTargetReader)
{
  if (aTargetReader == mAudioReader) {
    return false;
  }
  if (mAudioReader) {
    AudioInfo targetInfo = aTargetReader->GetMediaInfo().mAudio;
    AudioInfo currentInfo = mAudioReader->GetMediaInfo().mAudio;

    
    if (currentInfo.mRate != targetInfo.mRate ||
        currentInfo.mChannels != targetInfo.mChannels) {
      return false;
    }

    mAudioReader->SetIdle();
  }
  mAudioReader = aTargetReader;
  mDropAudioBeforeThreshold = true;
  MSE_DEBUG("MediaDecoderReader(%p)::SwitchReaders(%p) switching audio reader",
            this, mAudioReader.get());
  return true;
}

bool
MediaSourceReader::SwitchVideoReader(MediaDecoderReader* aTargetReader)
{
  if (aTargetReader == mVideoReader) {
    return false;
  }
  if (mVideoReader) {
    mVideoReader->SetIdle();
  }
  mVideoReader = aTargetReader;
  mDropVideoBeforeThreshold = true;
  MSE_DEBUG("MediaDecoderReader(%p)::SwitchVideoReader(%p) switching video reader",
            this, mVideoReader.get());
  return true;
}

bool
MediaSourceReader::SwitchReaders(SwitchType aType)
{
  InitializePendingDecoders();

  
  
  
  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  bool didSwitch = false;
  double decodeTarget = double(mTimeThreshold) / USECS_PER_S;

  for (uint32_t i = 0; i < mDecoders.Length(); ++i) {
    SourceBufferDecoder* decoder = mDecoders[i];
    const MediaInfo& info = decoder->GetReader()->GetMediaInfo();

    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    decoder->GetBuffered(ranges);

    MSE_DEBUGV("MediaDecoderReader(%p)::SwitchReaders(%d) decoder=%u (%p) discarded=%d"
               " reader=%p audioReader=%p videoReader=%p"
               " hasAudio=%d hasVideo=%d decodeTarget=%f ranges=%s",
               this, aType, i, decoder, decoder->IsDiscarded(),
               decoder->GetReader(), mAudioReader.get(), mVideoReader.get(),
               info.HasAudio(), info.HasVideo(), decodeTarget,
               DumpTimeRanges(ranges).get());

    if (decoder->IsDiscarded()) {
      continue;
    }

    if (aType == SWITCH_FORCED || ranges->Find(decodeTarget) != dom::TimeRanges::NoIndex) {
      if (info.HasAudio()) {
        didSwitch |= SwitchAudioReader(mDecoders[i]->GetReader());
      }
      if (info.HasVideo()) {
        didSwitch |= SwitchVideoReader(mDecoders[i]->GetReader());
      }
    }
  }

  return didSwitch;
}

class ReleaseDecodersTask : public nsRunnable {
public:
  ReleaseDecodersTask(nsTArray<nsRefPtr<SourceBufferDecoder>>& aDecoders)
  {
    mDecoders.SwapElements(aDecoders);
  }

  NS_IMETHOD Run() MOZ_OVERRIDE MOZ_FINAL {
    mDecoders.Clear();
    return NS_OK;
  }

private:
  nsTArray<nsRefPtr<SourceBufferDecoder>> mDecoders;
};

void
MediaSourceReader::InitializePendingDecoders()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  for (uint32_t i = 0; i < mPendingDecoders.Length(); ++i) {
    nsRefPtr<SourceBufferDecoder> decoder = mPendingDecoders[i];
    MediaDecoderReader* reader = decoder->GetReader();
    MSE_DEBUG("MediaSourceReader(%p): Initializing subdecoder %p reader %p",
              this, decoder.get(), reader);

    MediaInfo mi;
    nsAutoPtr<MetadataTags> tags; 
    nsresult rv;
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      rv = reader->ReadMetadata(&mi, getter_Transfers(tags));
    }
    reader->SetIdle();
    if (NS_FAILED(rv)) {
      
      MSE_DEBUG("MediaSourceReader(%p): Reader %p failed to initialize rv=%x", this, reader, rv);
      continue;
    }

    bool active = false;
    if (mi.HasVideo() || mi.HasAudio()) {
      MSE_DEBUG("MediaSourceReader(%p): Reader %p has video=%d audio=%d",
                this, reader, mi.HasVideo(), mi.HasAudio());
      if (mi.HasVideo()) {
        MSE_DEBUG("MediaSourceReader(%p): Reader %p video resolution=%dx%d",
                  this, reader, mi.mVideo.mDisplay.width, mi.mVideo.mDisplay.height);
      }
      if (mi.HasAudio()) {
        MSE_DEBUG("MediaSourceReader(%p): Reader %p audio sampleRate=%d channels=%d",
                  this, reader, mi.mAudio.mRate, mi.mAudio.mChannels);
      }
      active = true;
    }

    if (active) {
      mDecoders.AppendElement(decoder);
    } else {
      MSE_DEBUG("MediaSourceReader(%p): Reader %p not activated", this, reader);
    }
  }
  NS_DispatchToMainThread(new ReleaseDecodersTask(mPendingDecoders));
  MOZ_ASSERT(mPendingDecoders.IsEmpty());
  mDecoder->NotifyWaitingForResourcesStatusChanged();
}

MediaDecoderReader*
CreateReaderForType(const nsACString& aType, AbstractMediaDecoder* aDecoder)
{
#ifdef MOZ_FMP4
  
  
  
  
  if ((aType.LowerCaseEqualsLiteral("video/mp4") ||
       aType.LowerCaseEqualsLiteral("audio/mp4")) &&
      MP4Decoder::IsEnabled()) {
    return new MP4Reader(aDecoder);
  }
#endif
  return DecoderTraits::CreateReader(aType, aDecoder);
}

already_AddRefed<SourceBufferDecoder>
MediaSourceReader::CreateSubDecoder(const nsACString& aType)
{
  if (IsShutdown()) {
    return nullptr;
  }
  MOZ_ASSERT(GetTaskQueue());
  nsRefPtr<SourceBufferDecoder> decoder =
    new SourceBufferDecoder(new SourceBufferResource(aType), mDecoder);
  nsRefPtr<MediaDecoderReader> reader(CreateReaderForType(aType, decoder));
  if (!reader) {
    return nullptr;
  }
  
  
  
  RefPtr<MediaDataDecodedListener<MediaSourceReader>> callback =
    new MediaDataDecodedListener<MediaSourceReader>(this, GetTaskQueue());
  reader->SetCallback(callback);
  reader->SetTaskQueue(GetTaskQueue());
  reader->Init(nullptr);
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MSE_DEBUG("MediaSourceReader(%p)::CreateSubDecoder subdecoder %p subreader %p",
            this, decoder.get(), reader.get());
  decoder->SetReader(reader);
  mPendingDecoders.AppendElement(decoder);
  RefPtr<nsIRunnable> task =
    NS_NewRunnableMethod(this, &MediaSourceReader::InitializePendingDecoders);
  if (NS_FAILED(GetTaskQueue()->Dispatch(task))) {
    MSE_DEBUG("MediaSourceReader(%p): Failed to enqueue decoder initialization task", this);
    return nullptr;
  }
  mDecoder->NotifyWaitingForResourcesStatusChanged();
  return decoder.forget();
}

namespace {
class ChangeToHaveMetadata : public nsRunnable {
public:
  ChangeToHaveMetadata(AbstractMediaDecoder* aDecoder) :
    mDecoder(aDecoder)
  {
  }

  NS_IMETHOD Run() MOZ_OVERRIDE MOZ_FINAL {
    auto owner = mDecoder->GetOwner();
    if (owner) {
      owner->UpdateReadyStateForData(MediaDecoderOwner::NEXT_FRAME_WAIT_FOR_MSE_DATA);
    }
    return NS_OK;
  }

private:
  nsRefPtr<AbstractMediaDecoder> mDecoder;
};
}

bool
MediaSourceReader::DecodersContainTime(double aTime)
{
  bool found = false;

  for (uint32_t i = 0; i < mDecoders.Length(); ++i) {
    if (!mDecoders[i]->IsDiscarded()) {
      if (!mDecoders[i]->ContainsTime(aTime)) {
        
        return false;
      }
      found = true;
    }
  }
  return found;
}

nsresult
MediaSourceReader::Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
                        int64_t aCurrentTime)
{
  MSE_DEBUG("MediaSourceReader(%p)::Seek(aTime=%lld, aStart=%lld, aEnd=%lld, aCurrent=%lld)",
            this, aTime, aStartTime, aEndTime, aCurrentTime);
  double target = static_cast<double>(aTime) / USECS_PER_S;
  if (!DecodersContainTime(target)) {
    MSE_DEBUG("MediaSourceReader(%p)::Seek no active buffer contains target=%f", this, target);
    NS_DispatchToMainThread(new ChangeToHaveMetadata(mDecoder));
  }

  
  
  
  
  while (!DecodersContainTime(target) && !IsShutdown() && !IsEnded()) {
    MSE_DEBUG("MediaSourceReader(%p)::Seek waiting for target=%f", this, target);
    static_cast<MediaSourceDecoder*>(mDecoder)->WaitForData();
    SwitchReaders(SWITCH_FORCED);
  }

  if (IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  ResetDecode();
  if (mAudioReader) {
    nsresult rv = mAudioReader->Seek(aTime, aStartTime, aEndTime, aCurrentTime);
    MSE_DEBUG("MediaSourceReader(%p)::Seek audio reader=%p rv=%x", this, mAudioReader.get(), rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  if (mVideoReader) {
    nsresult rv = mVideoReader->Seek(aTime, aStartTime, aEndTime, aCurrentTime);
    MSE_DEBUG("MediaSourceReader(%p)::Seek video reader=%p rv=%x", this, mVideoReader.get(), rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  return NS_OK;
}

nsresult
MediaSourceReader::ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags)
{
  InitializePendingDecoders();

  MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata decoders=%u", this, mDecoders.Length());

  
  
  
  
  
  
  
  int64_t maxDuration = -1;
  for (uint32_t i = 0; i < mDecoders.Length(); ++i) {
    MediaDecoderReader* reader = mDecoders[i]->GetReader();

    MediaInfo mi = reader->GetMediaInfo();

    if (mi.HasVideo() && !mInfo.HasVideo()) {
      MOZ_ASSERT(!mVideoReader);
      mVideoReader = reader;
      mInfo.mVideo = mi.mVideo;
      maxDuration = std::max(maxDuration, mDecoders[i]->GetMediaDuration());
      MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata video reader=%p maxDuration=%lld",
                this, reader, maxDuration);
    }
    if (mi.HasAudio() && !mInfo.HasAudio()) {
      MOZ_ASSERT(!mAudioReader);
      mAudioReader = reader;
      mInfo.mAudio = mi.mAudio;
      maxDuration = std::max(maxDuration, mDecoders[i]->GetMediaDuration());
      MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata audio reader=%p maxDuration=%lld",
                this, reader, maxDuration);
    }
  }

  if (maxDuration != -1) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(maxDuration);
    nsRefPtr<nsIRunnable> task (
      NS_NewRunnableMethodWithArg<double>(static_cast<MediaSourceDecoder*>(mDecoder),
                                          &MediaSourceDecoder::SetMediaSourceDuration,
                                          static_cast<double>(maxDuration) / USECS_PER_S));
    NS_DispatchToMainThread(task);
  }

  *aInfo = mInfo;
  *aTags = nullptr; 

  return NS_OK;
}

void
MediaSourceReader::Ended()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mEnded = true;
}

bool
MediaSourceReader::IsEnded()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  return mEnded;
}

} 
