





#include "SpeechStreamListener.h"

#include "SpeechRecognition.h"
#include "nsProxyRelease.h"

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
                  static_cast<DOMEventTargetHelper*>(forgottenRecognition));
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
    
    if (iterator->GetDuration() > INT_MAX) {
      continue;
    }
    int duration = int(iterator->GetDuration());

    if (iterator->IsNull()) {
      nsTArray<int16_t> nullData;
      PodZero(nullData.AppendElements(duration), duration);
      ConvertAndDispatchAudioChunk(duration, iterator->mVolume, nullData.Elements(), aTrackRate);
    } else {
      AudioSampleFormat format = iterator->mBufferFormat;

      MOZ_ASSERT(format == AUDIO_FORMAT_S16 || format == AUDIO_FORMAT_FLOAT32);

      if (format == AUDIO_FORMAT_S16) {
        ConvertAndDispatchAudioChunk(duration,iterator->mVolume,
                                     static_cast<const int16_t*>(iterator->mChannelData[0]),
                                     aTrackRate);
      } else if (format == AUDIO_FORMAT_FLOAT32) {
        ConvertAndDispatchAudioChunk(duration,iterator->mVolume,
                                     static_cast<const float*>(iterator->mChannelData[0]),
                                     aTrackRate);
      }
    }

    iterator.Next();
  }
}

template<typename SampleFormatType> void
SpeechStreamListener::ConvertAndDispatchAudioChunk(int aDuration, float aVolume,
                                                   SampleFormatType* aData,
                                                   TrackRate aTrackRate)
{
  nsRefPtr<SharedBuffer> samples(SharedBuffer::Create(aDuration *
                                                      1 * 
                                                      sizeof(int16_t)));

  int16_t* to = static_cast<int16_t*>(samples->Data());
  ConvertAudioSamplesWithScale(aData, to, aDuration, aVolume);

  mRecognition->FeedAudioData(samples.forget(), aDuration, this, aTrackRate);
}

void
SpeechStreamListener::NotifyEvent(MediaStreamGraph* aGraph,
                                  MediaStreamListener::MediaStreamGraphEvent event)
{
  
}

} 
} 
