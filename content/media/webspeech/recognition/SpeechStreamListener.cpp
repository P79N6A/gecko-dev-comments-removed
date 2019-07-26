





#include "SpeechStreamListener.h"

#include "SpeechRecognition.h"

namespace mozilla {
namespace dom {

SpeechStreamListener::SpeechStreamListener(SpeechRecognition* aRecognition)
  : mRecognition(aRecognition)
{
}

SpeechStreamListener::~SpeechStreamListener()
{
  nsCOMPtr<nsIThread> mainThread;
  NS_GetMainThread(getter_AddRefs(mainThread));

  SpeechRecognition* forgottenRecognition = nullptr;
  mRecognition.swap(forgottenRecognition);
  NS_ProxyRelease(mainThread,
                  static_cast<nsDOMEventTargetHelper*>(forgottenRecognition));
}

void
SpeechStreamListener::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                               TrackID aID,
                                               TrackRate aTrackRate,
                                               TrackTicks aTrackOffset,
                                               uint32_t aTrackEvents,
                                               const MediaSegment& aQueuedMedia)
{
  AudioSegment* audio = const_cast<AudioSegment*>(
    static_cast<const AudioSegment*>(&aQueuedMedia));

  AudioSegment::ChunkIterator iterator(*audio);
  while (!iterator.IsEnded()) {
    AudioSampleFormat format = iterator->mBufferFormat;

    MOZ_ASSERT(format == AUDIO_FORMAT_S16 || format == AUDIO_FORMAT_FLOAT32);

    if (format == AUDIO_FORMAT_S16) {
      ConvertAndDispatchAudioChunk<int16_t>(*iterator);
    } else if (format == AUDIO_FORMAT_FLOAT32) {
      ConvertAndDispatchAudioChunk<float>(*iterator);
    }

    iterator.Next();
  }
}

template<typename SampleFormatType> void
SpeechStreamListener::ConvertAndDispatchAudioChunk(AudioChunk& aChunk)
{
  nsRefPtr<SharedBuffer> samples(SharedBuffer::Create(aChunk.mDuration *
                                                      1 * 
                                                      sizeof(int16_t)));

  const SampleFormatType* from =
    static_cast<const SampleFormatType*>(aChunk.mChannelData[0]);

  int16_t* to = static_cast<int16_t*>(samples->Data());
  ConvertAudioSamplesWithScale(from, to, aChunk.mDuration, aChunk.mVolume);

  mRecognition->FeedAudioData(samples.forget(), aChunk.mDuration, this);
  return;
}

void
SpeechStreamListener::NotifyFinished(MediaStreamGraph* aGraph)
{
  
}

} 
} 
