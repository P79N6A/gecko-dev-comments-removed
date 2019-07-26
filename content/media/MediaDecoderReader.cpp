





#include "MediaDecoderReader.h"
#include "AbstractMediaDecoder.h"
#include "VideoUtils.h"
#include "ImageContainer.h"

#include "mozilla/mozalloc.h"
#include <stdint.h>
#include <algorithm>

namespace mozilla {




#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#ifdef SEEK_LOGGING
#define SEEK_LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define SEEK_LOG(type, msg)
#endif
#else
#define DECODER_LOG(type, msg)
#define SEEK_LOG(type, msg)
#endif

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
  : mAudioCompactor(mAudioQueue),
    mDecoder(aDecoder),
    mIgnoreAudioOutputFormat(false)
{
  MOZ_COUNT_CTOR(MediaDecoderReader);
}

MediaDecoderReader::~MediaDecoderReader()
{
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

AudioData* MediaDecoderReader::DecodeToFirstAudioData()
{
  bool eof = false;
  while (!eof && AudioQueue().GetSize() == 0) {
    {
      ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
      if (mDecoder->IsShutdown()) {
        return nullptr;
      }
    }
    eof = !DecodeAudioData();
  }
  if (eof) {
    AudioQueue().Finish();
  }
  AudioData* d = nullptr;
  return (d = AudioQueue().PeekFront()) ? d : nullptr;
}

VideoData* MediaDecoderReader::FindStartTime(int64_t& aOutStartTime)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or decode thread.");

  
  
  int64_t videoStartTime = INT64_MAX;
  int64_t audioStartTime = INT64_MAX;
  VideoData* videoData = nullptr;

  if (HasVideo()) {
    videoData = DecodeToFirstVideoData();
    if (videoData) {
      videoStartTime = videoData->mTime;
      DECODER_LOG(PR_LOG_DEBUG, ("MediaDecoderReader::FindStartTime() video=%lld", videoStartTime));
    }
  }
  if (HasAudio()) {
    AudioData* audioData = DecodeToFirstAudioData();
    if (audioData) {
      audioStartTime = audioData->mTime;
      DECODER_LOG(PR_LOG_DEBUG, ("MediaDecoderReader::FindStartTime() audio=%lld", audioStartTime));
    }
  }

  int64_t startTime = std::min(videoStartTime, audioStartTime);
  if (startTime != INT64_MAX) {
    aOutStartTime = startTime;
  }

  return videoData;
}

nsresult MediaDecoderReader::DecodeToTarget(int64_t aTarget)
{
  DECODER_LOG(PR_LOG_DEBUG, ("MediaDecoderReader::DecodeToTarget(%lld) Begin", aTarget));

  
  if (HasVideo()) {
    
    
    
    bool eof = false;
    nsAutoPtr<VideoData> video;
    while (HasVideo() && !eof) {
      while (VideoQueue().GetSize() == 0 && !eof) {
        bool skip = false;
        eof = !DecodeVideoFrame(skip, 0);
        {
          ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
          if (mDecoder->IsShutdown()) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      if (eof) {
        
        if (video) {
          DECODER_LOG(PR_LOG_DEBUG,
            ("MediaDecoderReader::DecodeToTarget(%lld) repushing video frame [%lld, %lld] at EOF",
            aTarget, video->mTime, video->GetEndTime()));
          VideoQueue().PushFront(video.forget());
        }
        VideoQueue().Finish();
        break;
      }
      video = VideoQueue().PeekFront();
      
      
      if (video && video->GetEndTime() <= aTarget) {
        DECODER_LOG(PR_LOG_DEBUG,
                    ("MediaDecoderReader::DecodeToTarget(%lld) pop video frame [%lld, %lld]",
                     aTarget, video->mTime, video->GetEndTime()));
        VideoQueue().PopFront();
      } else {
        
        if (aTarget >= video->mTime && video->GetEndTime() >= aTarget) {
          
          
          
          VideoQueue().PopFront();
          VideoData* temp = VideoData::ShallowCopyUpdateTimestamp(video, aTarget);
          video = temp;
          VideoQueue().PushFront(video);
        }
        DECODER_LOG(PR_LOG_DEBUG,
                    ("MediaDecoderReader::DecodeToTarget(%lld) found target video frame [%lld,%lld]",
                     aTarget, video->mTime, video->GetEndTime()));

        video.forget();
        break;
      }
    }
    {
      ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
      if (mDecoder->IsShutdown()) {
        return NS_ERROR_FAILURE;
      }
    }
#ifdef PR_LOGGING
    const VideoData* front =  VideoQueue().PeekFront();
    DECODER_LOG(PR_LOG_DEBUG, ("First video frame after decode is %lld",
                front ? front->mTime : -1));
#endif
  }

  if (HasAudio()) {
    
    bool eof = false;
    while (HasAudio() && !eof) {
      while (!eof && AudioQueue().GetSize() == 0) {
        eof = !DecodeAudioData();
        {
          ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
          if (mDecoder->IsShutdown()) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      const AudioData* audio = AudioQueue().PeekFront();
      if (!audio || eof) {
        AudioQueue().Finish();
        break;
      }
      CheckedInt64 startFrame = UsecsToFrames(audio->mTime, mInfo.mAudio.mRate);
      CheckedInt64 targetFrame = UsecsToFrames(aTarget, mInfo.mAudio.mRate);
      if (!startFrame.isValid() || !targetFrame.isValid()) {
        return NS_ERROR_FAILURE;
      }
      if (startFrame.value() + audio->mFrames <= targetFrame.value()) {
        
        
        delete AudioQueue().PopFront();
        audio = nullptr;
        continue;
      }
      if (startFrame.value() > targetFrame.value()) {
        
        
        
        
        
        
        
        NS_WARNING("Audio not synced after seek, maybe a poorly muxed file?");
        break;
      }

      
      
      
      NS_ASSERTION(targetFrame.value() >= startFrame.value(),
                   "Target must at or be after data start.");
      NS_ASSERTION(targetFrame.value() < startFrame.value() + audio->mFrames,
                   "Data must end after target.");

      int64_t framesToPrune = targetFrame.value() - startFrame.value();
      if (framesToPrune > audio->mFrames) {
        
        
        NS_WARNING("Can't prune more frames that we have!");
        break;
      }
      uint32_t frames = audio->mFrames - static_cast<uint32_t>(framesToPrune);
      uint32_t channels = audio->mChannels;
      nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[frames * channels]);
      memcpy(audioData.get(),
             audio->mAudioData.get() + (framesToPrune * channels),
             frames * channels * sizeof(AudioDataValue));
      CheckedInt64 duration = FramesToUsecs(frames, mInfo.mAudio.mRate);
      if (!duration.isValid()) {
        return NS_ERROR_FAILURE;
      }
      nsAutoPtr<AudioData> data(new AudioData(audio->mOffset,
                                              aTarget,
                                              duration.value(),
                                              frames,
                                              audioData.forget(),
                                              channels));
      delete AudioQueue().PopFront();
      AudioQueue().PushFront(data.forget());
      break;
    }
  }

#ifdef PR_LOGGING
  const VideoData* v = VideoQueue().PeekFront();
  const AudioData* a = AudioQueue().PeekFront();
  DECODER_LOG(PR_LOG_DEBUG,
              ("MediaDecoderReader::DecodeToTarget(%lld) finished v=%lld a=%lld",
              aTarget, v ? v->mTime : -1, a ? a->mTime : -1));
#endif

  return NS_OK;
}

nsresult
MediaDecoderReader::GetBuffered(mozilla::dom::TimeRanges* aBuffered,
                                int64_t aStartTime)
{
  MediaResource* stream = mDecoder->GetResource();
  int64_t durationUs = 0;
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    durationUs = mDecoder->GetMediaDuration();
  }
  GetEstimatedBufferedTimeRanges(stream, durationUs, aBuffered);
  return NS_OK;
}

} 
