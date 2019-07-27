




#include "MediaSourceDecoder.h"

#include "prlog.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaDecoderStateMachine.h"
#include "MediaSource.h"
#include "MediaSourceReader.h"
#include "MediaSourceResource.h"
#include "MediaSourceUtils.h"
#include "SourceBufferDecoder.h"
#include "VideoUtils.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaSourceLog();

#define MSE_DEBUG(arg, ...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG, ("MediaSourceDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#define MSE_DEBUGV(arg, ...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG + 1, ("MediaSourceDecoder(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#define MSE_DEBUGV(...)
#endif

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
  return ScheduleStateMachineThread();
}

nsresult
MediaSourceDecoder::GetSeekable(dom::TimeRanges* aSeekable)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mMediaSource) {
    return NS_ERROR_FAILURE;
  }

  double duration = mMediaSource->Duration();
  if (IsNaN(duration)) {
    
  } else if (duration > 0 && mozilla::IsInfinite(duration)) {
    nsRefPtr<dom::TimeRanges> bufferedRanges = new dom::TimeRanges();
    mReader->GetBuffered(bufferedRanges);
    aSeekable->Add(bufferedRanges->GetStartTime(), bufferedRanges->GetEndTime());
  } else {
    aSeekable->Add(0, duration);
  }
  MSE_DEBUG("ranges=%s", DumpTimeRanges(aSeekable).get());
  return NS_OK;
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

class DurationChangedRunnable : public nsRunnable {
public:
  DurationChangedRunnable(MediaSourceDecoder* aDecoder,
                          double aOldDuration,
                          double aNewDuration)
    : mDecoder(aDecoder)
    , mOldDuration(aOldDuration)
    , mNewDuration(aNewDuration)
  { }

  NS_IMETHOD Run() override final {
    mDecoder->DurationChanged(mOldDuration, mNewDuration);
    return NS_OK;
  }

private:
  RefPtr<MediaSourceDecoder> mDecoder;
  double mOldDuration;
  double mNewDuration;
};

void
MediaSourceDecoder::DurationChanged(double aOldDuration, double aNewDuration)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  if (mMediaSource) {
    mMediaSource->DurationChange(aOldDuration, aNewDuration);
  }
  
  MediaDecoder::DurationChanged();
}

void
MediaSourceDecoder::SetInitialDuration(int64_t aDuration)
{
  
  
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
  ScheduleDurationChange(oldDuration, aDuration, aAction);
}

void
MediaSourceDecoder::ScheduleDurationChange(double aOldDuration,
                                           double aNewDuration,
                                           MSRangeRemovalAction aAction)
{
  if (aAction == MSRangeRemovalAction::SKIP) {
    if (NS_IsMainThread()) {
      MediaDecoder::DurationChanged();
    } else {
      nsCOMPtr<nsIRunnable> task =
        NS_NewRunnableMethod(this, &MediaDecoder::DurationChanged);
      NS_DispatchToMainThread(task);
    }
  } else {
    if (NS_IsMainThread()) {
      DurationChanged(aOldDuration, aNewDuration);
    } else {
      nsCOMPtr<nsIRunnable> task =
        new DurationChangedRunnable(this, aOldDuration, aNewDuration);
      NS_DispatchToMainThread(task);
    }
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

  
  
  for (int32_t i = aTrackDecoders.Length() - 1; i >= 0; --i) {
    nsRefPtr<SourceBufferDecoder> newDecoder = aTrackDecoders[i];

    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    newDecoder->GetBuffered(ranges);
    if (ranges->Find(double(aTarget) / USECS_PER_S,
                     double(aTolerance) / USECS_PER_S) == dom::TimeRanges::NoIndex) {
      MSE_DEBUGV("SelectDecoder(%lld fuzz:%lld) newDecoder=%p (%d/%d) target not in ranges=%s",
                 aTarget, aTolerance, newDecoder.get(), i+1,
                 aTrackDecoders.Length(), DumpTimeRanges(ranges).get());
      continue;
    }

    return newDecoder.forget();
  }

  return nullptr;
}

#undef MSE_DEBUG
#undef MSE_DEBUGV

} 
