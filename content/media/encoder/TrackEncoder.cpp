



#include "TrackEncoder.h"
#include "MediaStreamGraph.h"
#include "AudioChannelFormat.h"

#undef LOG
#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, "MediakEncoder", ## args);
#else
#define LOG(args, ...)
#endif

namespace mozilla {

#define MAX_FRAMES_TO_DROP  48000

void
AudioTrackEncoder::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                            TrackID aID,
                                            TrackRate aTrackRate,
                                            TrackTicks aTrackOffset,
                                            uint32_t aTrackEvents,
                                            const MediaSegment& aQueuedMedia)
{
  AudioSegment* audio = const_cast<AudioSegment*>
                        (static_cast<const AudioSegment*>(&aQueuedMedia));

  
  if (!mInitialized) {
    AudioSegment::ChunkIterator iter(*audio);
    while (!iter.IsEnded()) {
      AudioChunk chunk = *iter;
      if (chunk.mBuffer) {
        Init(chunk.mChannelData.Length(), aTrackRate);
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
AudioTrackEncoder::NotifyRemoved(MediaStreamGraph* aGraph)
{
  
  LOG("[AudioTrackEncoder]: NotifyRemoved.");
  NotifyEndOfStream();
}

nsresult
AudioTrackEncoder::AppendAudioSegment(MediaSegment* aSegment)
{
  
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  AudioSegment* audio = static_cast<AudioSegment*>(aSegment);
  AudioSegment::ChunkIterator iter(*audio);

  if (mRawSegment->GetDuration() < MAX_FRAMES_TO_DROP) {
    while(!iter.IsEnded()) {
      AudioChunk chunk = *iter;
      if (chunk.mBuffer) {
        mRawSegment->AppendAndConsumeChunk(&chunk);
      }
      iter.Next();
    }
    if (mRawSegment->GetDuration() >= GetPacketDuration()) {
      mReentrantMonitor.NotifyAll();
    }
  }
#ifdef DEBUG
  else {
    LOG("[AudioTrackEncoder]: A segment has dropped!");
  }
#endif

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
                         aChunk.mVolume, mChannels, aOutput);
  } else {
    InterleaveAndConvertBuffer(aChunk.mChannelData.Elements(),
                               aChunk.mBufferFormat, aDuration, aChunk.mVolume,
                               mChannels, aOutput);
  }
}

}
