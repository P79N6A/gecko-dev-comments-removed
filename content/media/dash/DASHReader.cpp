













#include "nsTimeRanges.h"
#include "VideoFrameContainer.h"
#include "AbstractMediaDecoder.h"
#include "DASHReader.h"

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define LOG(msg, ...) PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, \
                             ("%p [DASHReader] " msg, this, __VA_ARGS__))
#define LOG1(msg) PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, \
                         ("%p [DASHReader] " msg, this))
#else
#define LOG(msg, ...)
#define LOG1(msg)
#endif

nsresult
DASHReader::Init(MediaDecoderReader* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(mAudioReaders.Length() != 0 && mVideoReaders.Length() != 0,
               "Audio and video readers should exist already.");

  nsresult rv;
  for (uint i = 0; i < mAudioReaders.Length(); i++) {
    rv = mAudioReaders[i]->Init(nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  for (uint i = 0; i < mVideoReaders.Length(); i++) {
    rv = mVideoReaders[i]->Init(nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

void
DASHReader::AddAudioReader(MediaDecoderReader* aAudioReader)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(aAudioReader, );

  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mAudioReaders.AppendElement(aAudioReader);
  
  if (!mAudioReader)
    mAudioReader = aAudioReader;
}

void
DASHReader::AddVideoReader(MediaDecoderReader* aVideoReader)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(aVideoReader, );

  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mVideoReaders.AppendElement(aVideoReader);
  
  if (!mVideoReader)
    mVideoReader = aVideoReader;
}

int64_t
DASHReader::VideoQueueMemoryInUse()
{
  ReentrantMonitorConditionallyEnter mon(!mDecoder->OnDecodeThread(),
                                         mDecoder->GetReentrantMonitor());
  return (mVideoReader ? mVideoReader->VideoQueueMemoryInUse() : 0);
}

int64_t
DASHReader::AudioQueueMemoryInUse()
{
  ReentrantMonitorConditionallyEnter mon(!mDecoder->OnDecodeThread(),
                                         mDecoder->GetReentrantMonitor());
  return (mAudioReader ? mAudioReader->AudioQueueMemoryInUse() : 0);
}

bool
DASHReader::DecodeVideoFrame(bool &aKeyframeSkip,
                               int64_t aTimeThreshold)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  if (mVideoReader) {
   return mVideoReader->DecodeVideoFrame(aKeyframeSkip, aTimeThreshold);
  } else {
   return false;
  }
}

bool
DASHReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  return (mAudioReader ? mAudioReader->DecodeAudioData() : false);
}

nsresult
DASHReader::ReadMetadata(VideoInfo* aInfo,
                           MetadataTags** aTags)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  
  LOG1("Waiting for metadata download.");
  nsresult rv = WaitForMetadata();
  
  if (NS_ERROR_ABORT == rv) {
    return NS_OK;
  }
  
  NS_ENSURE_SUCCESS(rv, rv);

  
  VideoInfo audioInfo, videoInfo;

  if (mVideoReader) {
    rv = mVideoReader->ReadMetadata(&videoInfo, aTags);
    NS_ENSURE_SUCCESS(rv, rv);
    mInfo.mHasVideo      = videoInfo.mHasVideo;
    mInfo.mDisplay       = videoInfo.mDisplay;
  }
  if (mAudioReader) {
    rv = mAudioReader->ReadMetadata(&audioInfo, aTags);
    NS_ENSURE_SUCCESS(rv, rv);
    mInfo.mHasAudio      = audioInfo.mHasAudio;
    mInfo.mAudioRate     = audioInfo.mAudioRate;
    mInfo.mAudioChannels = audioInfo.mAudioChannels;
    mInfo.mStereoMode    = audioInfo.mStereoMode;
  }

  *aInfo = mInfo;

  return NS_OK;
}

nsresult
DASHReader::Seek(int64_t aTime,
                   int64_t aStartTime,
                   int64_t aEndTime,
                   int64_t aCurrentTime)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  nsresult rv;

  if (mAudioReader) {
    rv = mAudioReader->Seek(aTime, aStartTime, aEndTime, aCurrentTime);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (mVideoReader) {
    rv = mVideoReader->Seek(aTime, aStartTime, aEndTime, aCurrentTime);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
DASHReader::GetBuffered(nsTimeRanges* aBuffered,
                          int64_t aStartTime)
{
  NS_ENSURE_ARG(aBuffered);

  MediaResource* resource = nullptr;
  AbstractMediaDecoder* decoder = nullptr;

  
  nsTimeRanges audioBuffered, videoBuffered;
  uint32_t audioRangeCount, videoRangeCount;

  nsresult rv = NS_OK;

  
  ReentrantMonitorConditionallyEnter mon(!mDecoder->OnDecodeThread(),
                                         mDecoder->GetReentrantMonitor());
  if (mAudioReader) {
    decoder = mAudioReader->GetDecoder();
    NS_ENSURE_TRUE(decoder, NS_ERROR_NULL_POINTER);
    resource = decoder->GetResource();
    NS_ENSURE_TRUE(resource, NS_ERROR_NULL_POINTER);
    resource->Pin();
    rv = mAudioReader->GetBuffered(&audioBuffered, aStartTime);
    NS_ENSURE_SUCCESS(rv, rv);
    resource->Unpin();
    rv = audioBuffered.GetLength(&audioRangeCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (mVideoReader) {
    decoder = mVideoReader->GetDecoder();
    NS_ENSURE_TRUE(decoder, NS_ERROR_NULL_POINTER);
    resource = decoder->GetResource();
    NS_ENSURE_TRUE(resource, NS_ERROR_NULL_POINTER);
    resource->Pin();
    rv = mVideoReader->GetBuffered(&videoBuffered, aStartTime);
    NS_ENSURE_SUCCESS(rv, rv);
    resource->Unpin();
    rv = videoBuffered.GetLength(&videoRangeCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (mAudioReader && mVideoReader) {
    
    for (uint32_t i = 0; i < audioRangeCount; i++) {
      
      double startA, startV, startI;
      double endA, endV, endI;
      rv = audioBuffered.Start(i, &startA);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = audioBuffered.End(i, &endA);
      NS_ENSURE_SUCCESS(rv, rv);

      for (uint32_t j = 0; j < videoRangeCount; j++) {
        rv = videoBuffered.Start(i, &startV);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = videoBuffered.End(i, &endV);
        NS_ENSURE_SUCCESS(rv, rv);

        
        if (startA > endV) {
          continue;
        
        
        } else if (endA < startV) {
          break;
        }
        
        startI = (startA > startV) ? startA : startV;
        endI = (endA > endV) ? endV : endA;
        aBuffered->Add(startI, endI);
      }
    }
  } else if (mAudioReader) {
    *aBuffered = audioBuffered;
  } else if (mVideoReader) {
    *aBuffered = videoBuffered;
  } else {
    return NS_ERROR_NOT_INITIALIZED;
  }

  return NS_OK;
}

VideoData*
DASHReader::FindStartTime(int64_t& aOutStartTime)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or decode thread.");

  
  
  int64_t videoStartTime = INT64_MAX;
  int64_t audioStartTime = INT64_MAX;
  VideoData* videoData = nullptr;

  ReentrantMonitorConditionallyEnter mon(!mDecoder->OnDecodeThread(),
                                         mDecoder->GetReentrantMonitor());
  if (HasVideo()) {
    
    videoData = mVideoReader->DecodeToFirstVideoData();
    if (videoData) {
      videoStartTime = videoData->mTime;
    }
  }
  if (HasAudio()) {
    
    AudioData* audioData = mAudioReader->DecodeToFirstAudioData();
    if (audioData) {
      audioStartTime = audioData->mTime;
    }
  }

  int64_t startTime = NS_MIN(videoStartTime, audioStartTime);
  if (startTime != INT64_MAX) {
    aOutStartTime = startTime;
  }

  return videoData;
}

MediaQueue<AudioData>&
DASHReader::AudioQueue()
{
  ReentrantMonitorConditionallyEnter mon(!mDecoder->OnDecodeThread(),
                                         mDecoder->GetReentrantMonitor());
  NS_ASSERTION(mAudioReader, "mAudioReader is NULL!");
  return mAudioReader->AudioQueue();
}

MediaQueue<VideoData>&
DASHReader::VideoQueue()
{
  ReentrantMonitorConditionallyEnter mon(!mDecoder->OnDecodeThread(),
                                         mDecoder->GetReentrantMonitor());
  NS_ASSERTION(mVideoReader, "mVideoReader is NULL!");
  return mVideoReader->VideoQueue();
}

bool
DASHReader::IsSeekableInBufferedRanges()
{
  ReentrantMonitorConditionallyEnter mon(!mDecoder->OnDecodeThread(),
                                         mDecoder->GetReentrantMonitor());
  
  return (mVideoReader || mAudioReader) &&
          !((mVideoReader && !mVideoReader->IsSeekableInBufferedRanges()) ||
            (mAudioReader && !mAudioReader->IsSeekableInBufferedRanges()));
}

} 

