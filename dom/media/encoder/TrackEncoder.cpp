



#include "TrackEncoder.h"
#include "AudioChannelFormat.h"
#include "MediaStreamGraph.h"
#include "mozilla/Logging.h"
#include "VideoUtils.h"

#undef LOG
#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, "MediaEncoder", ## args);
#else
#define LOG(args, ...)
#endif

namespace mozilla {

PRLogModuleInfo* gTrackEncoderLog;
#define TRACK_LOG(type, msg) MOZ_LOG(gTrackEncoderLog, type, msg)

static const int DEFAULT_CHANNELS = 1;
static const int DEFAULT_SAMPLING_RATE = 16000;
static const int DEFAULT_FRAME_WIDTH = 640;
static const int DEFAULT_FRAME_HEIGHT = 480;
static const int DEFAULT_TRACK_RATE = USECS_PER_S;

TrackEncoder::TrackEncoder()
  : mReentrantMonitor("media.TrackEncoder")
  , mEncodingComplete(false)
  , mEosSetInEncoder(false)
  , mInitialized(false)
  , mEndOfStream(false)
  , mCanceled(false)
  , mAudioInitCounter(0)
  , mVideoInitCounter(0)
{
  if (!gTrackEncoderLog) {
    gTrackEncoderLog = PR_NewLogModule("TrackEncoder");
  }
}

void
AudioTrackEncoder::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                            TrackID aID,
                                            StreamTime aTrackOffset,
                                            uint32_t aTrackEvents,
                                            const MediaSegment& aQueuedMedia)
{
  if (mCanceled) {
    return;
  }

  const AudioSegment& audio = static_cast<const AudioSegment&>(aQueuedMedia);

  
  if (!mInitialized) {
    mAudioInitCounter++;
    TRACK_LOG(LogLevel::Debug, ("Init the audio encoder %d times", mAudioInitCounter));
    AudioSegment::ChunkIterator iter(const_cast<AudioSegment&>(audio));
    while (!iter.IsEnded()) {
      AudioChunk chunk = *iter;

      
      
      if (!chunk.IsNull()) {
        nsresult rv = Init(chunk.mChannelData.Length(), aGraph->GraphRate());
        if (NS_FAILED(rv)) {
          LOG("[AudioTrackEncoder]: Fail to initialize the encoder!");
          NotifyCancel();
        }
        break;
      }

      iter.Next();
    }
  }

  
  AppendAudioSegment(audio);


  
  if (aTrackEvents == MediaStreamListener::TRACK_EVENT_ENDED) {
    LOG("[AudioTrackEncoder]: Receive TRACK_EVENT_ENDED .");
    NotifyEndOfStream();
  }
}

void
AudioTrackEncoder::NotifyEndOfStream()
{
  
  
  if (!mCanceled && !mInitialized) {
    Init(DEFAULT_CHANNELS, DEFAULT_SAMPLING_RATE);
  }

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mEndOfStream = true;
  mReentrantMonitor.NotifyAll();
}

nsresult
AudioTrackEncoder::AppendAudioSegment(const AudioSegment& aSegment)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  AudioSegment::ChunkIterator iter(const_cast<AudioSegment&>(aSegment));
  while (!iter.IsEnded()) {
    AudioChunk chunk = *iter;
    
    mRawSegment.AppendAndConsumeChunk(&chunk);
    iter.Next();
  }

  if (mRawSegment.GetDuration() >= GetPacketDuration()) {
    mReentrantMonitor.NotifyAll();
  }

  return NS_OK;
}

static const int AUDIO_PROCESSING_FRAMES = 640; 
static const uint8_t gZeroChannel[MAX_AUDIO_SAMPLE_SIZE*AUDIO_PROCESSING_FRAMES] = {0};


void
AudioTrackEncoder::InterleaveTrackData(AudioChunk& aChunk,
                                       int32_t aDuration,
                                       uint32_t aOutputChannels,
                                       AudioDataValue* aOutput)
{
  if (aChunk.mChannelData.Length() < aOutputChannels) {
    
    AudioChannelsUpMix(&aChunk.mChannelData, aOutputChannels, gZeroChannel);
  }

  if (aChunk.mChannelData.Length() > aOutputChannels) {
    DownmixAndInterleave(aChunk.mChannelData, aChunk.mBufferFormat, aDuration,
                         aChunk.mVolume, aOutputChannels, aOutput);
  } else {
    InterleaveAndConvertBuffer(aChunk.mChannelData.Elements(),
                               aChunk.mBufferFormat, aDuration, aChunk.mVolume,
                               aOutputChannels, aOutput);
  }
}


void
AudioTrackEncoder::DeInterleaveTrackData(AudioDataValue* aInput,
                                         int32_t aDuration,
                                         int32_t aChannels,
                                         AudioDataValue* aOutput)
{
  for (int32_t i = 0; i < aChannels; ++i) {
    for(int32_t j = 0; j < aDuration; ++j) {
      aOutput[i * aDuration + j] = aInput[i + j * aChannels];
    }
  }
}

size_t
AudioTrackEncoder::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return mRawSegment.SizeOfExcludingThis(aMallocSizeOf);
}

void
VideoTrackEncoder::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                            TrackID aID,
                                            StreamTime aTrackOffset,
                                            uint32_t aTrackEvents,
                                            const MediaSegment& aQueuedMedia)
{
  if (mCanceled) {
    return;
  }

  const VideoSegment& video = static_cast<const VideoSegment&>(aQueuedMedia);

   
  if (!mInitialized) {
    mVideoInitCounter++;
    TRACK_LOG(LogLevel::Debug, ("Init the video encoder %d times", mVideoInitCounter));
    VideoSegment::ChunkIterator iter(const_cast<VideoSegment&>(video));
    while (!iter.IsEnded()) {
      VideoChunk chunk = *iter;
      if (!chunk.IsNull()) {
        gfx::IntSize imgsize = chunk.mFrame.GetImage()->GetSize();
        gfxIntSize intrinsicSize = chunk.mFrame.GetIntrinsicSize();
        nsresult rv = Init(imgsize.width, imgsize.height,
                           intrinsicSize.width, intrinsicSize.height,
                           aGraph->GraphRate());
        if (NS_FAILED(rv)) {
          LOG("[VideoTrackEncoder]: Fail to initialize the encoder!");
          NotifyCancel();
        }
        break;
      }

      iter.Next();
    }
  }

  AppendVideoSegment(video);

  
  if (aTrackEvents == MediaStreamListener::TRACK_EVENT_ENDED) {
    LOG("[VideoTrackEncoder]: Receive TRACK_EVENT_ENDED .");
    NotifyEndOfStream();
  }

}

nsresult
VideoTrackEncoder::AppendVideoSegment(const VideoSegment& aSegment)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  
  
  VideoSegment::ChunkIterator iter(const_cast<VideoSegment&>(aSegment));
  while (!iter.IsEnded()) {
    VideoChunk chunk = *iter;
    nsRefPtr<layers::Image> image = chunk.mFrame.GetImage();
    mRawSegment.AppendFrame(image.forget(),
                            chunk.GetDuration(),
                            chunk.mFrame.GetIntrinsicSize(),
                            chunk.mFrame.GetForceBlack());
    iter.Next();
  }

  if (mRawSegment.GetDuration() > 0) {
    mReentrantMonitor.NotifyAll();
  }

  return NS_OK;
}

void
VideoTrackEncoder::NotifyEndOfStream()
{
  
  
  if (!mCanceled && !mInitialized) {
    Init(DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT,
         DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT, DEFAULT_TRACK_RATE);
  }

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mEndOfStream = true;
  mReentrantMonitor.NotifyAll();
}

size_t
VideoTrackEncoder::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return mRawSegment.SizeOfExcludingThis(aMallocSizeOf);
}

} 
