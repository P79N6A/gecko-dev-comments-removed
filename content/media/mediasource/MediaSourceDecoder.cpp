




#include "MediaSourceDecoder.h"

#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TimeRanges.h"
#include "MediaDecoderStateMachine.h"
#include "MediaSource.h"
#include "MediaSourceReader.h"
#include "MediaSourceResource.h"

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

class SubBufferDecoder;

class MediaSourceStateMachine : public MediaDecoderStateMachine
{
public:
  MediaSourceStateMachine(MediaDecoder* aDecoder,
                          MediaDecoderReader* aReader,
                          bool aRealTime = false)
    : MediaDecoderStateMachine(aDecoder, aReader, aRealTime)
  {
  }

  already_AddRefed<SubBufferDecoder> CreateSubDecoder(const nsACString& aType,
                                                      MediaSourceDecoder* aParentDecoder) {
    if (!mReader) {
      return nullptr;
    }
    MediaSourceReader* reader = static_cast<MediaSourceReader*>(mReader.get());
    return reader->CreateSubDecoder(aType, aParentDecoder, mDecodeTaskQueue);
  }

  nsresult EnqueueDecoderInitialization() {
    AssertCurrentThreadInMonitor();
    if (!mReader) {
      return NS_ERROR_FAILURE;
    }
    RefPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this, &MediaSourceStateMachine::InitializePendingDecoders);
    return mDecodeTaskQueue->Dispatch(task);
  }

private:
  void InitializePendingDecoders() {
    if (!mReader) {
      return;
    }
    MediaSourceReader* reader = static_cast<MediaSourceReader*>(mReader.get());
    reader->InitializePendingDecoders();
  }
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
  return new MediaSourceStateMachine(this, new MediaSourceReader(this, mMediaSource));
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

  return rv;
}

nsresult
MediaSourceDecoder::GetSeekable(dom::TimeRanges* aSeekable)
{
  double duration = mMediaSource->Duration();
  if (IsNaN(duration)) {
    
  } else if (duration > 0 && mozilla::IsInfinite(duration)) {
    nsRefPtr<dom::TimeRanges> bufferedRanges = new dom::TimeRanges();
    mMediaSource->GetBuffered(bufferedRanges);
    aSeekable->Add(bufferedRanges->GetStartTime(), bufferedRanges->GetEndTime());
  } else {
    aSeekable->Add(0, duration);
  }
  MSE_DEBUG("MediaSourceDecoder(%p)::GetSeekable startTime=%f endTime=%f",
            this, aSeekable->GetStartTime(), aSeekable->GetEndTime());
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
  if (!mDecoderStateMachine) {
    return nullptr;
  }
  MediaSourceStateMachine* sm = static_cast<MediaSourceStateMachine*>(mDecoderStateMachine.get());
  return sm->CreateSubDecoder(aType, this);
}

nsresult
MediaSourceDecoder::EnqueueDecoderInitialization()
{
  if (!mDecoderStateMachine) {
    return NS_ERROR_FAILURE;
  }
  MediaSourceStateMachine* sm = static_cast<MediaSourceStateMachine*>(mDecoderStateMachine.get());
  return sm->EnqueueDecoderInitialization();
}

} 
