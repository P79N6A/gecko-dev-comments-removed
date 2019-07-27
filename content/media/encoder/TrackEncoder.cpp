



#include "TrackEncoder.h"
#include "AudioChannelFormat.h"
#include "MediaStreamGraph.h"
#include "prlog.h"
#include "VideoUtils.h"

#undef LOG
#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, "MediaEncoder", ## args);
#else
#define LOG(args, ...)
#endif

namespace mozilla {

#ifdef PR_LOGGING
PRLogModuleInfo* gTrackEncoderLog;
#define TRACK_LOG(type, msg) PR_LOG(gTrackEncoderLog, type, msg)
#else
#define TRACK_LOG(type, msg)
#endif

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
#ifdef PR_LOGGING
  , mAudioInitCounter(0)
  , mVideoInitCounter(0)
#endif
{
#ifdef PR_LOGGING
  if (!gTrackEncoderLog) {
    gTrackEncoderLog = PR_NewLogModule("TrackEncoder");
  }
#endif
}

void
AudioTrackEncoder::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                            TrackID aID,
                                            TrackRate aTrackRate,
                                            TrackTicks aTrackOffset,
                                            uint32_t aTrackEvents,
                                            const MediaSegment& aQueuedMedia)
{
  if (mCanceled) {
    return;
  }

  const AudioSegment& audio = static_cast<const AudioSegment&>(aQueuedMedia);

  
  if (!mInitialized) {
#ifdef PR_LOGGING
    mAudioInitCounter++;
    TRACK_LOG(PR_LOG_DEBUG, ("Init the audio encoder %d times", mAudioInitCounter));
#endif
    AudioSegment::ChunkIterator iter(const_cast<AudioSegment&>(audio));
    while (!iter.IsEnded()) {
      AudioChunk chunk = *iter;

      
      
      if (!chunk.IsNull()) {
        nsresult rv = Init(chunk.mChannelData.Length(), aTrackRate);
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

void
VideoTrackEncoder::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                            TrackID aID,
                                            TrackRate aTrackRate,
                                            TrackTicks aTrackOffset,
                                            uint32_t aTrackEvents,
                                            const MediaSegment& aQueuedMedia)
{
  if (mCanceled) {
    return;
  }

  const VideoSegment& video = static_cast<const VideoSegment&>(aQueuedMedia);

   
  if (!mInitialized) {
#ifdef PR_LOGGING
    mVideoInitCounter++;
    TRACK_LOG(PR_LOG_DEBUG, ("Init the video encoder %d times", mVideoInitCounter));
#endif
    VideoSegment::ChunkIterator iter(const_cast<VideoSegment&>(video));
    while (!iter.IsEnded()) {
      VideoChunk chunk = *iter;
      if (!chunk.IsNull()) {
        gfx::IntSize imgsize = chunk.mFrame.GetImage()->GetSize();
        gfxIntSize intrinsicSize = chunk.mFrame.GetIntrinsicSize();
#ifdef MOZ_WIDGET_GONK
        
        if (chunk.mFrame.GetImage()->GetFormat() != ImageFormat::PLANAR_YCBCR) {
          LOG("Can't encode this ImageFormat %x", chunk.mFrame.GetImage()->GetFormat());
          NotifyCancel();
          break;
        }
#endif
        nsresult rv = Init(imgsize.width, imgsize.height,
                           intrinsicSize.width, intrinsicSize.height,
                           aTrackRate);
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
                            chunk.mFrame.GetIntrinsicSize().ToIntSize(),
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

}
