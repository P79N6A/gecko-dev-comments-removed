




#include "MediaSourceDecoder.h"

#include "prlog.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaDecoderStateMachine.h"
#include "MediaSource.h"
#include "MediaSourceReader.h"
#include "MediaSourceResource.h"
#include "MediaSourceUtils.h"

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
  MOZ_ASSERT(!mDecoderStateMachine);
  mDecoderStateMachine = CreateStateMachine();
  if (!mDecoderStateMachine) {
    NS_WARNING("Failed to create state machine!");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = mDecoderStateMachine->Init(nullptr);
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
  MSE_DEBUG("MediaSourceDecoder(%p)::GetSeekable ranges=%s", this, DumpTimeRanges(aSeekable).get());
  return NS_OK;
}

void
MediaSourceDecoder::Shutdown()
{
  MSE_DEBUG("MediaSourceDecoder(%p)::Shutdown", this);
  
  
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
  MOZ_ASSERT(!mMediaSource && !mDecoderStateMachine && NS_IsMainThread());
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
MediaSourceDecoder::Ended()
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mReader->Ended();
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
  explicit DurationChangedRunnable(MediaSourceDecoder* aDecoder,
                                   double aOldDuration,
                                   double aNewDuration)
    : mDecoder(aDecoder)
    , mOldDuration(aOldDuration)
    , mNewDuration(aNewDuration)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE MOZ_FINAL {
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
MediaSourceDecoder::SetDecodedDuration(int64_t aDuration)
{
  
  
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (!mMediaSource || !IsNaN(mMediaSourceDuration)) {
    return;
  }
  double duration = aDuration;
  duration /= USECS_PER_S;
  SetMediaSourceDuration(duration);
}

void
MediaSourceDecoder::SetMediaSourceDuration(double aDuration)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  double oldDuration = mMediaSourceDuration;
  mMediaSourceDuration = aDuration;
  mDecoderStateMachine->SetDuration(aDuration * USECS_PER_S);
  if (NS_IsMainThread()) {
    DurationChanged(oldDuration, aDuration);
  } else {
    nsRefPtr<nsIRunnable> task =
      new DurationChangedRunnable(this, oldDuration, aDuration);
    NS_DispatchToMainThread(task);
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

#ifdef MOZ_EME
nsresult
MediaSourceDecoder::SetCDMProxy(CDMProxy* aProxy)
{
  nsresult rv = MediaDecoder::SetCDMProxy(aProxy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mReader->SetCDMProxy(aProxy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
#endif

} 
