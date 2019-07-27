




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

extern PRLogModuleInfo* GetMediaSourceLog();

#define MSE_DEBUG(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Debug, ("MediaSourceDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#define MSE_DEBUGV(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Verbose, ("MediaSourceDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))

namespace mozilla {

class SourceBufferDecoder;

MediaSourceDecoder::MediaSourceDecoder(dom::HTMLMediaElement* aElement)
  : mMediaSource(nullptr)
  , mMediaSourceDuration(UnspecifiedNaN<double>())
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
    media::TimeIntervals buffered = mReader->GetBuffered();
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
  MOZ_ASSERT(mReader);
  return mReader->CreateSubDecoder(aType, aTimestampOffset);
}

void
MediaSourceDecoder::AddTrackBuffer(TrackBuffer* aTrackBuffer)
{
  MOZ_ASSERT(mReader);
  mReader->AddTrackBuffer(aTrackBuffer);
}

void
MediaSourceDecoder::RemoveTrackBuffer(TrackBuffer* aTrackBuffer)
{
  MOZ_ASSERT(mReader);
  mReader->RemoveTrackBuffer(aTrackBuffer);
}

void
MediaSourceDecoder::OnTrackBufferConfigured(TrackBuffer* aTrackBuffer, const MediaInfo& aInfo)
{
  MOZ_ASSERT(mReader);
  mReader->OnTrackBufferConfigured(aTrackBuffer, aInfo);
}

void
MediaSourceDecoder::Ended(bool aEnded)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  static_cast<MediaSourceResource*>(GetResource())->SetEnded(aEnded);
  mReader->Ended(aEnded);
  mon.NotifyAll();
}

bool
MediaSourceDecoder::IsExpectingMoreData()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return !mReader->IsEnded();
}

void
MediaSourceDecoder::SetInitialDuration(int64_t aDuration)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (!mMediaSource || !IsNaN(mMediaSourceDuration)) {
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
  double oldDuration = mMediaSourceDuration;
  if (aDuration >= 0) {
    int64_t checkedDuration;
    if (NS_FAILED(SecondsToUsecs(aDuration, checkedDuration))) {
      
      
      checkedDuration = INT64_MAX - 1;
    }
    GetStateMachine()->SetDuration(checkedDuration);
    mMediaSourceDuration = aDuration;
  } else {
    GetStateMachine()->SetDuration(INT64_MAX);
    mMediaSourceDuration = PositiveInfinity<double>();
  }
  if (mReader) {
    mReader->SetMediaSourceDuration(mMediaSourceDuration);
  }

  MediaDecoder::DurationChanged();
  if (mMediaSource && aAction != MSRangeRemovalAction::SKIP) {
    mMediaSource->DurationChange(oldDuration, aDuration);
  }
}

double
MediaSourceDecoder::GetMediaSourceDuration()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return mMediaSourceDuration;
}

void
MediaSourceDecoder::NotifyTimeRangesChanged()
{
  MOZ_ASSERT(mReader);
  mReader->NotifyTimeRangesChanged();
}

void
MediaSourceDecoder::PrepareReaderInitialization()
{
  MOZ_ASSERT(mReader);
  mReader->PrepareInitialization();
}

void
MediaSourceDecoder::GetMozDebugReaderData(nsAString& aString)
{
  mReader->GetMozDebugReaderData(aString);
}

#ifdef MOZ_EME
nsresult
MediaSourceDecoder::SetCDMProxy(CDMProxy* aProxy)
{
  nsresult rv = MediaDecoder::SetCDMProxy(aProxy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mReader->SetCDMProxy(aProxy);
  NS_ENSURE_SUCCESS(rv, rv);

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
  return mReader->IsActiveReader(aReader);
}

double
MediaSourceDecoder::GetDuration()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return mMediaSourceDuration;
}

already_AddRefed<SourceBufferDecoder>
MediaSourceDecoder::SelectDecoder(int64_t aTarget,
                                  int64_t aTolerance,
                                  const nsTArray<nsRefPtr<SourceBufferDecoder>>& aTrackDecoders)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  media::TimeUnit target{media::TimeUnit::FromMicroseconds(aTarget)};
  media::TimeUnit tolerance{media::TimeUnit::FromMicroseconds(aTolerance + aTarget)};

  
  
  
  for (int32_t i = aTrackDecoders.Length() - 1; i >= 0; --i) {
    nsRefPtr<SourceBufferDecoder> newDecoder = aTrackDecoders[i];

    media::TimeIntervals ranges = newDecoder->GetBuffered();
    for (uint32_t j = 0; j < ranges.Length(); j++) {
      if (target < ranges.End(j) && tolerance >= ranges.Start(j)) {
        return newDecoder.forget();
      }
    }
    MSE_DEBUGV("SelectDecoder(%lld fuzz:%lld) newDecoder=%p (%d/%d) target not in ranges=%s",
               aTarget, aTolerance, newDecoder.get(), i+1,
               aTrackDecoders.Length(), DumpTimeRanges(ranges).get());
  }

  return nullptr;
}

#undef MSE_DEBUG
#undef MSE_DEBUGV

} 
