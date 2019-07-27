





#include "MediaDecoderReader.h"
#include "AbstractMediaDecoder.h"
#include "MediaResource.h"
#include "VideoUtils.h"
#include "ImageContainer.h"

#include "nsPrintfCString.h"
#include "mozilla/mozalloc.h"
#include <stdint.h>
#include <algorithm>

using namespace mozilla::media;

namespace mozilla {




extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(x, ...) \
  MOZ_LOG(gMediaDecoderLog, LogLevel::Debug, ("Decoder=%p " x, mDecoder, ##__VA_ARGS__))


#define DECODER_WARN_HELPER(a, b) NS_WARNING b
#define DECODER_WARN(x, ...) \
  DECODER_WARN_HELPER(0, (nsPrintfCString("Decoder=%p " x, mDecoder, ##__VA_ARGS__).get()))

class VideoQueueMemoryFunctor : public nsDequeFunctor {
public:
  VideoQueueMemoryFunctor() : mSize(0) {}

  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf);

  virtual void* operator()(void* aObject) {
    const VideoData* v = static_cast<const VideoData*>(aObject);
    mSize += v->SizeOfIncludingThis(MallocSizeOf);
    return nullptr;
  }

  size_t mSize;
};


class AudioQueueMemoryFunctor : public nsDequeFunctor {
public:
  AudioQueueMemoryFunctor() : mSize(0) {}

  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf);

  virtual void* operator()(void* aObject) {
    const AudioData* audioData = static_cast<const AudioData*>(aObject);
    mSize += audioData->SizeOfIncludingThis(MallocSizeOf);
    return nullptr;
  }

  size_t mSize;
};

MediaDecoderReader::MediaDecoderReader(AbstractMediaDecoder* aDecoder,
                                       MediaTaskQueue* aBorrowedTaskQueue)
  : mAudioCompactor(mAudioQueue)
  , mDecoder(aDecoder)
  , mTaskQueue(aBorrowedTaskQueue ? aBorrowedTaskQueue
                                  : new MediaTaskQueue(GetMediaThreadPool(MediaThreadType::PLAYBACK),
                                                        true))
  , mWatchManager(this, mTaskQueue)
  , mTimer(new MediaTimer())
  , mBuffered(mTaskQueue, TimeIntervals(), "MediaDecoderReader::mBuffered (Canonical)")
  , mDuration(mTaskQueue, NullableTimeUnit(), "MediaDecoderReader::mDuration (Mirror)")
  , mThrottleDuration(TimeDuration::FromMilliseconds(500))
  , mLastThrottledNotify(TimeStamp::Now() - mThrottleDuration)
  , mIgnoreAudioOutputFormat(false)
  , mHitAudioDecodeError(false)
  , mShutdown(false)
  , mTaskQueueIsBorrowed(!!aBorrowedTaskQueue)
  , mAudioDiscontinuity(false)
  , mVideoDiscontinuity(false)
{
  MOZ_COUNT_CTOR(MediaDecoderReader);
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(this, &MediaDecoderReader::InitializationTask);
  mTaskQueue->Dispatch(r.forget());
}

void
MediaDecoderReader::InitializationTask()
{
  if (mDecoder->CanonicalDurationOrNull()) {
    mDuration.Connect(mDecoder->CanonicalDurationOrNull());
  }

  
  mWatchManager.Watch(mDuration, &MediaDecoderReader::UpdateBuffered);
}

MediaDecoderReader::~MediaDecoderReader()
{
  MOZ_ASSERT(mShutdown);
  MOZ_ASSERT(!mDecoder);
  ResetDecode();
  MOZ_COUNT_DTOR(MediaDecoderReader);
}

size_t MediaDecoderReader::SizeOfVideoQueueInBytes() const
{
  VideoQueueMemoryFunctor functor;
  mVideoQueue.LockedForEach(functor);
  return functor.mSize;
}

size_t MediaDecoderReader::SizeOfAudioQueueInBytes() const
{
  AudioQueueMemoryFunctor functor;
  mAudioQueue.LockedForEach(functor);
  return functor.mSize;
}

size_t MediaDecoderReader::SizeOfVideoQueueInFrames()
{
  return mVideoQueue.GetSize();
}

size_t MediaDecoderReader::SizeOfAudioQueueInFrames()
{
  return mAudioQueue.GetSize();
}

nsresult MediaDecoderReader::ResetDecode()
{
  VideoQueue().Reset();
  AudioQueue().Reset();

  mAudioDiscontinuity = true;
  mVideoDiscontinuity = true;

  mBaseAudioPromise.RejectIfExists(CANCELED, __func__);
  mBaseVideoPromise.RejectIfExists(CANCELED, __func__);

  return NS_OK;
}

VideoData* MediaDecoderReader::DecodeToFirstVideoData()
{
  bool eof = false;
  while (!eof && VideoQueue().GetSize() == 0) {
    {
      ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
      if (mDecoder->IsShutdown()) {
        return nullptr;
      }
    }
    bool keyframeSkip = false;
    eof = !DecodeVideoFrame(keyframeSkip, 0);
  }
  if (eof) {
    VideoQueue().Finish();
  }
  VideoData* d = nullptr;
  return (d = VideoQueue().PeekFront()) ? d : nullptr;
}

void
MediaDecoderReader::UpdateBuffered()
{
  MOZ_ASSERT(OnTaskQueue());
  NS_ENSURE_TRUE_VOID(!mShutdown);
  mBuffered = GetBuffered();
}

void
MediaDecoderReader::ThrottledNotifyDataArrived(const Interval<int64_t>& aInterval)
{
  MOZ_ASSERT(OnTaskQueue());
  NS_ENSURE_TRUE_VOID(!mShutdown);

  if (mThrottledInterval.isNothing()) {
    mThrottledInterval.emplace(aInterval);
  } else if (!mThrottledInterval.ref().Contiguous(aInterval)) {
    DoThrottledNotify();
    mThrottledInterval.emplace(aInterval);
  } else {
    mThrottledInterval = Some(mThrottledInterval.ref().Span(aInterval));
  }

  
  if (TimeStamp::Now() - mLastThrottledNotify > mThrottleDuration) {
    DoThrottledNotify();
  } else if (!mThrottledNotify.Exists()) {
    
    nsRefPtr<MediaDecoderReader> self = this;
    mThrottledNotify.Begin(
      mTimer->WaitUntil(mLastThrottledNotify + mThrottleDuration, __func__)
      ->Then(TaskQueue(), __func__,
             [self] () -> void {
               self->mThrottledNotify.Complete();
               NS_ENSURE_TRUE_VOID(!self->mShutdown);
               self->DoThrottledNotify();
             },
             [self] () -> void {
               self->mThrottledNotify.Complete();
               NS_WARNING("throttle callback rejected");
             })
    );
  }
}

void
MediaDecoderReader::DoThrottledNotify()
{
  MOZ_ASSERT(OnTaskQueue());
  mLastThrottledNotify = TimeStamp::Now();
  mThrottledNotify.DisconnectIfExists();
  Interval<int64_t> interval = mThrottledInterval.ref();
  mThrottledInterval.reset();
  NotifyDataArrived(interval);
}

media::TimeIntervals
MediaDecoderReader::GetBuffered()
{
  MOZ_ASSERT(OnTaskQueue());
  NS_ENSURE_TRUE(HaveStartTime(), media::TimeIntervals());
  AutoPinned<MediaResource> stream(mDecoder->GetResource());

  if (!mDuration.Ref().isSome()) {
    return TimeIntervals();
  }

  return GetEstimatedBufferedTimeRanges(stream, mDuration.Ref().ref().ToMicroseconds());
}

nsRefPtr<MediaDecoderReader::MetadataPromise>
MediaDecoderReader::AsyncReadMetadata()
{
  typedef ReadMetadataFailureReason Reason;

  MOZ_ASSERT(OnTaskQueue());
  mDecoder->GetReentrantMonitor().AssertNotCurrentThreadIn();
  DECODER_LOG("MediaDecoderReader::AsyncReadMetadata");

  if (IsWaitingMediaResources()) {
    return MetadataPromise::CreateAndReject(Reason::WAITING_FOR_RESOURCES, __func__);
  }

  
  nsRefPtr<MetadataHolder> metadata = new MetadataHolder();
  nsresult rv = ReadMetadata(&metadata->mInfo, getter_Transfers(metadata->mTags));

  
  
  if (IsWaitingMediaResources()) {
    return MetadataPromise::CreateAndReject(Reason::WAITING_FOR_RESOURCES, __func__);
  }

  
  
  if (NS_FAILED(rv) || !metadata->mInfo.HasValidMedia()) {
    DECODER_WARN("ReadMetadata failed, rv=%x HasValidMedia=%d", rv, metadata->mInfo.HasValidMedia());
    return MetadataPromise::CreateAndReject(Reason::METADATA_ERROR, __func__);
  }

  
  return MetadataPromise::CreateAndResolve(metadata, __func__);
}

class ReRequestVideoWithSkipTask : public nsRunnable
{
public:
  ReRequestVideoWithSkipTask(MediaDecoderReader* aReader,
                             int64_t aTimeThreshold)
    : mReader(aReader)
    , mTimeThreshold(aTimeThreshold)
  {
  }

  NS_METHOD Run()
  {
    MOZ_ASSERT(mReader->OnTaskQueue());

    
    if (!mReader->mBaseVideoPromise.IsEmpty()) {
      mReader->RequestVideoData( true, mTimeThreshold);
    }

    return NS_OK;
  }

private:
  nsRefPtr<MediaDecoderReader> mReader;
  const int64_t mTimeThreshold;
};

class ReRequestAudioTask : public nsRunnable
{
public:
  explicit ReRequestAudioTask(MediaDecoderReader* aReader)
    : mReader(aReader)
  {
  }

  NS_METHOD Run()
  {
    MOZ_ASSERT(mReader->OnTaskQueue());

    
    if (!mReader->mBaseAudioPromise.IsEmpty()) {
      mReader->RequestAudioData();
    }

    return NS_OK;
  }

private:
  nsRefPtr<MediaDecoderReader> mReader;
};

nsRefPtr<MediaDecoderReader::VideoDataPromise>
MediaDecoderReader::RequestVideoData(bool aSkipToNextKeyframe,
                                     int64_t aTimeThreshold)
{
  nsRefPtr<VideoDataPromise> p = mBaseVideoPromise.Ensure(__func__);
  bool skip = aSkipToNextKeyframe;
  while (VideoQueue().GetSize() == 0 &&
         !VideoQueue().IsFinished()) {
    if (!DecodeVideoFrame(skip, aTimeThreshold)) {
      VideoQueue().Finish();
    } else if (skip) {
      
      
      
      
      RefPtr<nsIRunnable> task(new ReRequestVideoWithSkipTask(this, aTimeThreshold));
      mTaskQueue->Dispatch(task.forget());
      return p;
    }
  }
  if (VideoQueue().GetSize() > 0) {
    nsRefPtr<VideoData> v = VideoQueue().PopFront();
    if (v && mVideoDiscontinuity) {
      v->mDiscontinuity = true;
      mVideoDiscontinuity = false;
    }
    mBaseVideoPromise.Resolve(v, __func__);
  } else if (VideoQueue().IsFinished()) {
    mBaseVideoPromise.Reject(END_OF_STREAM, __func__);
  } else {
    MOZ_ASSERT(false, "Dropping this promise on the floor");
  }

  return p;
}

nsRefPtr<MediaDecoderReader::AudioDataPromise>
MediaDecoderReader::RequestAudioData()
{
  nsRefPtr<AudioDataPromise> p = mBaseAudioPromise.Ensure(__func__);
  while (AudioQueue().GetSize() == 0 &&
         !AudioQueue().IsFinished()) {
    if (!DecodeAudioData()) {
      AudioQueue().Finish();
      break;
    }
    
    
    
    
    if (AudioQueue().GetSize() == 0 && mTaskQueue) {
      RefPtr<nsIRunnable> task(new ReRequestAudioTask(this));
      mTaskQueue->Dispatch(task.forget());
      return p;
    }
  }
  if (AudioQueue().GetSize() > 0) {
    nsRefPtr<AudioData> a = AudioQueue().PopFront();
    if (mAudioDiscontinuity) {
      a->mDiscontinuity = true;
      mAudioDiscontinuity = false;
    }
    mBaseAudioPromise.Resolve(a, __func__);
  } else if (AudioQueue().IsFinished()) {
    mBaseAudioPromise.Reject(mHitAudioDecodeError ? DECODE_ERROR : END_OF_STREAM, __func__);
    mHitAudioDecodeError = false;
  } else {
    MOZ_ASSERT(false, "Dropping this promise on the floor");
  }

  return p;
}

void
MediaDecoderReader::BreakCycles()
{
  
  
  
}

nsRefPtr<ShutdownPromise>
MediaDecoderReader::Shutdown()
{
  MOZ_ASSERT(OnTaskQueue());
  mShutdown = true;

  mBaseAudioPromise.RejectIfExists(END_OF_STREAM, __func__);
  mBaseVideoPromise.RejectIfExists(END_OF_STREAM, __func__);

  mThrottledNotify.DisconnectIfExists();

  ReleaseMediaResources();
  mDuration.DisconnectIfConnected();
  mBuffered.DisconnectAll();

  
  mWatchManager.Shutdown();

  nsRefPtr<ShutdownPromise> p;

  
  
  
  if (mTaskQueue && !mTaskQueueIsBorrowed) {
    
    p = mTaskQueue->BeginShutdown();
  } else {
    
    
    p = ShutdownPromise::CreateAndResolve(true, __func__);
  }

  mTimer = nullptr;
  mDecoder = nullptr;

  return p;
}

} 

#undef DECODER_LOG
#undef DECODER_WARN
#undef DECODER_WARN_HELPER
