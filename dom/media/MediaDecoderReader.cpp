





#include "MediaDecoderReader.h"
#include "AbstractMediaDecoder.h"
#include "MediaResource.h"
#include "VideoUtils.h"
#include "ImageContainer.h"

#include "mozilla/mozalloc.h"
#include <stdint.h>
#include <algorithm>

namespace mozilla {




#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(x, ...) \
  PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, ("Decoder=%p " x, mDecoder, ##__VA_ARGS__))
#else
#define DECODER_LOG(x, ...)
#endif

PRLogModuleInfo* gMediaPromiseLog;

void
EnsureMediaPromiseLog()
{
  if (!gMediaPromiseLog) {
    gMediaPromiseLog = PR_NewLogModule("MediaPromise");
  }
}

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

MediaDecoderReader::MediaDecoderReader(AbstractMediaDecoder* aDecoder)
  : mAudioCompactor(mAudioQueue)
  , mDecoder(aDecoder)
  , mIgnoreAudioOutputFormat(false)
  , mStartTime(-1)
  , mTaskQueueIsBorrowed(false)
  , mAudioDiscontinuity(false)
  , mVideoDiscontinuity(false)
  , mShutdown(false)
{
  MOZ_COUNT_CTOR(MediaDecoderReader);
  EnsureMediaPromiseLog();
}

MediaDecoderReader::~MediaDecoderReader()
{
  MOZ_ASSERT(mShutdown);
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

nsresult MediaDecoderReader::ResetDecode()
{
  nsresult res = NS_OK;

  VideoQueue().Reset();
  AudioQueue().Reset();

  mAudioDiscontinuity = true;
  mVideoDiscontinuity = true;

  return res;
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
MediaDecoderReader::SetStartTime(int64_t aStartTime)
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mStartTime = aStartTime;
}

nsresult
MediaDecoderReader::GetBuffered(mozilla::dom::TimeRanges* aBuffered)
{
  AutoPinned<MediaResource> stream(mDecoder->GetResource());
  int64_t durationUs = 0;
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    durationUs = mDecoder->GetMediaDuration();
  }
  GetEstimatedBufferedTimeRanges(stream, durationUs, aBuffered);
  return NS_OK;
}

int64_t
MediaDecoderReader::ComputeStartTime(const VideoData* aVideo, const AudioData* aAudio)
{
  int64_t startTime = std::min<int64_t>(aAudio ? aAudio->mTime : INT64_MAX,
                                        aVideo ? aVideo->mTime : INT64_MAX);
  if (startTime == INT64_MAX) {
    startTime = 0;
  }
  DECODER_LOG("ComputeStartTime first video frame start %lld", aVideo ? aVideo->mTime : -1);
  DECODER_LOG("ComputeStartTime first audio frame start %lld", aAudio ? aAudio->mTime : -1);
  MOZ_ASSERT(startTime >= 0);
  return startTime;
}

class RequestVideoWithSkipTask : public nsRunnable {
public:
  RequestVideoWithSkipTask(MediaDecoderReader* aReader,
                           int64_t aTimeThreshold)
    : mReader(aReader)
    , mTimeThreshold(aTimeThreshold)
  {
  }
  NS_METHOD Run() {
    bool skip = true;
    mReader->RequestVideoData(skip, mTimeThreshold);
    return NS_OK;
  }
private:
  nsRefPtr<MediaDecoderReader> mReader;
  int64_t mTimeThreshold;
};

void
MediaDecoderReader::RequestVideoData(bool aSkipToNextKeyframe,
                                     int64_t aTimeThreshold)
{
  bool skip = aSkipToNextKeyframe;
  while (VideoQueue().GetSize() == 0 &&
         !VideoQueue().IsFinished()) {
    if (!DecodeVideoFrame(skip, aTimeThreshold)) {
      VideoQueue().Finish();
    } else if (skip) {
      
      
      
      
      RefPtr<nsIRunnable> task(new RequestVideoWithSkipTask(this, aTimeThreshold));
      mTaskQueue->Dispatch(task);
      return;
    }
  }
  if (VideoQueue().GetSize() > 0) {
    nsRefPtr<VideoData> v = VideoQueue().PopFront();
    if (v && mVideoDiscontinuity) {
      v->mDiscontinuity = true;
      mVideoDiscontinuity = false;
    }
    GetCallback()->OnVideoDecoded(v);
  } else if (VideoQueue().IsFinished()) {
    GetCallback()->OnNotDecoded(MediaData::VIDEO_DATA, END_OF_STREAM);
  }
}

void
MediaDecoderReader::RequestAudioData()
{
  while (AudioQueue().GetSize() == 0 &&
         !AudioQueue().IsFinished()) {
    if (!DecodeAudioData()) {
      AudioQueue().Finish();
      break;
    }
    
    
    
    
    if (AudioQueue().GetSize() == 0 && mTaskQueue) {
      RefPtr<nsIRunnable> task(NS_NewRunnableMethod(
          this, &MediaDecoderReader::RequestAudioData));
      mTaskQueue->Dispatch(task.forget());
      return;
    }
  }
  if (AudioQueue().GetSize() > 0) {
    nsRefPtr<AudioData> a = AudioQueue().PopFront();
    if (mAudioDiscontinuity) {
      a->mDiscontinuity = true;
      mAudioDiscontinuity = false;
    }
    GetCallback()->OnAudioDecoded(a);
    return;
  } else if (AudioQueue().IsFinished()) {
    GetCallback()->OnNotDecoded(MediaData::AUDIO_DATA, END_OF_STREAM);
    return;
  }
}

void
MediaDecoderReader::SetCallback(RequestSampleCallback* aCallback)
{
  mSampleDecodedCallback = aCallback;
}

MediaTaskQueue*
MediaDecoderReader::EnsureTaskQueue()
{
  if (!mTaskQueue) {
    MOZ_ASSERT(!mTaskQueueIsBorrowed);
    RefPtr<SharedThreadPool> decodePool(GetMediaDecodeThreadPool());
    NS_ENSURE_TRUE(decodePool, nullptr);

    mTaskQueue = new MediaTaskQueue(decodePool.forget());
  }

  return mTaskQueue;
}

void
MediaDecoderReader::BreakCycles()
{
  if (mSampleDecodedCallback) {
    mSampleDecodedCallback->BreakCycles();
    mSampleDecodedCallback = nullptr;
  }
  mTaskQueue = nullptr;
}

nsRefPtr<ShutdownPromise>
MediaDecoderReader::Shutdown()
{
  MOZ_ASSERT(OnDecodeThread());
  mShutdown = true;
  ReleaseMediaResources();
  nsRefPtr<ShutdownPromise> p;

  
  
  
  if (mTaskQueue && !mTaskQueueIsBorrowed) {
    
    p = mTaskQueue->BeginShutdown();
  } else {
    
    
    p = new ShutdownPromise(__func__);
    p->Resolve(true, __func__);
  }

  return p;
}

AudioDecodeRendezvous::AudioDecodeRendezvous()
  : mMonitor("AudioDecodeRendezvous")
  , mHaveResult(false)
{
}

AudioDecodeRendezvous::~AudioDecodeRendezvous()
{
}

void
AudioDecodeRendezvous::OnAudioDecoded(AudioData* aSample)
{
  MonitorAutoLock mon(mMonitor);
  mSample = aSample;
  mStatus = NS_OK;
  mHaveResult = true;
  mon.NotifyAll();
}

void
AudioDecodeRendezvous::OnNotDecoded(MediaData::Type aType,
                                    MediaDecoderReader::NotDecodedReason aReason)
{
  MOZ_ASSERT(aType == MediaData::AUDIO_DATA);
  MonitorAutoLock mon(mMonitor);
  mSample = nullptr;
  mStatus = aReason == MediaDecoderReader::DECODE_ERROR ? NS_ERROR_FAILURE : NS_OK;
  mHaveResult = true;
  mon.NotifyAll();
}

void
AudioDecodeRendezvous::Reset()
{
  MonitorAutoLock mon(mMonitor);
  mHaveResult = false;
  mStatus = NS_OK;
  mSample = nullptr;
}

nsresult
AudioDecodeRendezvous::Await(nsRefPtr<AudioData>& aSample)
{
  MonitorAutoLock mon(mMonitor);
  while (!mHaveResult) {
    mon.Wait();
  }
  mHaveResult = false;
  aSample = mSample;
  return mStatus;
}

void
AudioDecodeRendezvous::Cancel()
{
  MonitorAutoLock mon(mMonitor);
  mStatus = NS_ERROR_ABORT;
  mHaveResult = true;
  mon.NotifyAll();
}

} 
