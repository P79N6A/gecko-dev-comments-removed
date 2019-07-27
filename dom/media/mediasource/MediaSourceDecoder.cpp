




#include "MediaSourceDecoder.h"

#include "mozilla/Logging.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "MediaDecoderStateMachine.h"
#include "MediaSource.h"
#include "MediaSourceReader.h"
#include "MediaSourceResource.h"
#include "MediaSourceUtils.h"
#include "SourceBufferDecoder.h"
#include "VideoUtils.h"
#include "MediaFormatReader.h"
#include "MediaSourceDemuxer.h"

extern PRLogModuleInfo* GetMediaSourceLog();

#define MSE_DEBUG(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Debug, ("MediaSourceDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#define MSE_DEBUGV(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Verbose, ("MediaSourceDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))

using namespace mozilla::media;

namespace mozilla {

class SourceBufferDecoder;

MediaSourceDecoder::MediaSourceDecoder(dom::HTMLMediaElement* aElement)
  : mMediaSource(nullptr)
  , mIsUsingFormatReader(Preferences::GetBool("media.mediasource.format-reader", false))
  , mEnded(false)
{
  SetExplicitDuration(UnspecifiedNaN<double>());
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
  if (mIsUsingFormatReader) {
    mDemuxer = new MediaSourceDemuxer();
    mReader = new MediaFormatReader(this, mDemuxer);
  } else {
    mReader = new MediaSourceReader(this);
  }
  return new MediaDecoderStateMachine(this, mReader);
}

nsresult
MediaSourceDecoder::Load(nsIStreamListener**, MediaDecoder*)
{
  MOZ_ASSERT(!GetStateMachine());
  SetStateMachine(CreateStateMachine());
  if (!GetStateMachine()) {
    NS_WARNING("Failed to create state machine!");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = GetStateMachine()->Init(nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  SetStateMachineParameters();
  return ScheduleStateMachine();
}

media::TimeIntervals
MediaSourceDecoder::GetSeekable()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mMediaSource) {
    NS_WARNING("MediaSource element isn't attached");
    return media::TimeIntervals::Invalid();
  }

  media::TimeIntervals seekable;
  double duration = mMediaSource->Duration();
  if (IsNaN(duration)) {
    
  } else if (duration > 0 && mozilla::IsInfinite(duration)) {
    media::TimeIntervals buffered = GetBuffered();
    if (buffered.Length()) {
      seekable += media::TimeInterval(buffered.GetStart(), buffered.GetEnd());
    }
  } else {
    seekable += media::TimeInterval(media::TimeUnit::FromSeconds(0),
                                    media::TimeUnit::FromSeconds(duration));
  }
  MSE_DEBUG("ranges=%s", DumpTimeRanges(seekable).get());
  return seekable;
}

media::TimeIntervals
MediaSourceDecoder::GetBuffered()
{
  MOZ_ASSERT(NS_IsMainThread());

  dom::SourceBufferList* sourceBuffers = mMediaSource->ActiveSourceBuffers();
  media::TimeUnit highestEndTime;
  nsTArray<media::TimeIntervals> activeRanges;
  media::TimeIntervals buffered;

  for (uint32_t i = 0; i < sourceBuffers->Length(); i++) {
    bool found;
    dom::SourceBuffer* sb = sourceBuffers->IndexedGetter(i, found);
    MOZ_ASSERT(found);

    activeRanges.AppendElement(sb->GetTimeIntervals());
    highestEndTime =
      std::max(highestEndTime, activeRanges.LastElement().GetEnd());
  }

  buffered +=
    media::TimeInterval(media::TimeUnit::FromMicroseconds(0), highestEndTime);

  for (auto& range : activeRanges) {
    if (mEnded && range.Length()) {
      
      
      
      range +=
        media::TimeInterval(range.GetEnd(), highestEndTime);
    }
    buffered.Intersection(range);
  }

  MSE_DEBUG("ranges=%s", DumpTimeRanges(buffered).get());
  return buffered;
}

void
MediaSourceDecoder::Shutdown()
{
  MSE_DEBUG("Shutdown");
  
  
  if (mMediaSource) {
    mMediaSource->Detach();
  }

  MediaDecoder::Shutdown();
  
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mon.NotifyAll();
}


already_AddRefed<MediaResource>
MediaSourceDecoder::CreateResource(nsIPrincipal* aPrincipal)
{
  return nsRefPtr<MediaResource>(new MediaSourceResource(aPrincipal)).forget();
}

void
MediaSourceDecoder::AttachMediaSource(dom::MediaSource* aMediaSource)
{
  MOZ_ASSERT(!mMediaSource && !GetStateMachine() && NS_IsMainThread());
  mMediaSource = aMediaSource;
}

void
MediaSourceDecoder::DetachMediaSource()
{
  MOZ_ASSERT(mMediaSource && NS_IsMainThread());
  mMediaSource = nullptr;
}

already_AddRefed<SourceBufferDecoder>
MediaSourceDecoder::CreateSubDecoder(const nsACString& aType, int64_t aTimestampOffset)
{
  MOZ_ASSERT(mReader && !mIsUsingFormatReader);
  return GetReader()->CreateSubDecoder(aType, aTimestampOffset);
}

void
MediaSourceDecoder::AddTrackBuffer(TrackBuffer* aTrackBuffer)
{
  MOZ_ASSERT(mReader && !mIsUsingFormatReader);
  GetReader()->AddTrackBuffer(aTrackBuffer);
}

void
MediaSourceDecoder::RemoveTrackBuffer(TrackBuffer* aTrackBuffer)
{
  MOZ_ASSERT(mReader && !mIsUsingFormatReader);
  GetReader()->RemoveTrackBuffer(aTrackBuffer);
}

void
MediaSourceDecoder::OnTrackBufferConfigured(TrackBuffer* aTrackBuffer, const MediaInfo& aInfo)
{
  MOZ_ASSERT(mReader && !mIsUsingFormatReader);
  GetReader()->OnTrackBufferConfigured(aTrackBuffer, aInfo);
}

void
MediaSourceDecoder::Ended(bool aEnded)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  static_cast<MediaSourceResource*>(GetResource())->SetEnded(aEnded);
  if (!mIsUsingFormatReader) {
    GetReader()->Ended(aEnded);
  }
  mEnded = true;
  mon.NotifyAll();
}

bool
MediaSourceDecoder::IsExpectingMoreData()
{
  return !mEnded;
}

void
MediaSourceDecoder::SetInitialDuration(int64_t aDuration)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (!mMediaSource || !IsNaN(ExplicitDuration())) {
    return;
  }
  double duration = aDuration;
  
  if (aDuration >= 0) {
    duration /= USECS_PER_S;
  }
  SetMediaSourceDuration(duration, MSRangeRemovalAction::SKIP);
}

void
MediaSourceDecoder::SetMediaSourceDuration(double aDuration, MSRangeRemovalAction aAction)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  double oldDuration = ExplicitDuration();
  if (aDuration >= 0) {
    int64_t checkedDuration;
    if (NS_FAILED(SecondsToUsecs(aDuration, checkedDuration))) {
      
      
      checkedDuration = INT64_MAX - 1;
    }
    SetExplicitDuration(aDuration);
  } else {
    SetExplicitDuration(PositiveInfinity<double>());
  }
  if (!mIsUsingFormatReader && GetReader()) {
    GetReader()->SetMediaSourceDuration(ExplicitDuration());
  }

  if (mMediaSource && aAction != MSRangeRemovalAction::SKIP) {
    mMediaSource->DurationChange(oldDuration, aDuration);
  }
}

double
MediaSourceDecoder::GetMediaSourceDuration()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return ExplicitDuration();
}

void
MediaSourceDecoder::NotifyTimeRangesChanged()
{
  MOZ_ASSERT(mReader && !mIsUsingFormatReader);
  GetReader()->NotifyTimeRangesChanged();
}

void
MediaSourceDecoder::PrepareReaderInitialization()
{
  if (mIsUsingFormatReader) {
    return;
  }
  MOZ_ASSERT(mReader);
  GetReader()->PrepareInitialization();
}

void
MediaSourceDecoder::GetMozDebugReaderData(nsAString& aString)
{
  if (mIsUsingFormatReader) {
    return;
  }
  GetReader()->GetMozDebugReaderData(aString);
}

#ifdef MOZ_EME
nsresult
MediaSourceDecoder::SetCDMProxy(CDMProxy* aProxy)
{
  nsresult rv = MediaDecoder::SetCDMProxy(aProxy);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mIsUsingFormatReader) {
    rv = GetReader()->SetCDMProxy(aProxy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (aProxy) {
    
    
    
    
    CDMCaps::AutoLock caps(aProxy->Capabilites());
    if (!caps.AreCapsKnown()) {
      nsCOMPtr<nsIRunnable> task(
        NS_NewRunnableMethod(this, &MediaDecoder::NotifyWaitingForResourcesStatusChanged));
      caps.CallOnMainThreadWhenCapsAvailable(task);
    }
  }
  return NS_OK;
}
#endif

bool
MediaSourceDecoder::IsActiveReader(MediaDecoderReader* aReader)
{
  return !mIsUsingFormatReader && GetReader()->IsActiveReader(aReader);
}

double
MediaSourceDecoder::GetDuration()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return ExplicitDuration();
}

#undef MSE_DEBUG
#undef MSE_DEBUGV

} 
