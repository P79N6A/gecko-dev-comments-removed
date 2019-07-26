





#pragma once

#include "MediaStreamGraph.h"
#include "AudioSegment.h"

namespace mozilla {

class AudioSegment;

namespace dom {

class SpeechRecognition;

class SpeechStreamListener : public MediaStreamListener
{
public:
  SpeechStreamListener(SpeechRecognition* aRecognition);
  ~SpeechStreamListener();

  void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                TrackRate aTrackRate,
                                TrackTicks aTrackOffset,
                                uint32_t aTrackEvents,
                                const MediaSegment& aQueuedMedia);

  void NotifyFinished(MediaStreamGraph* aGraph);

private:
  template<typename SampleFormatType> void ConvertAndDispatchAudioChunk(AudioChunk& aChunk);
  nsRefPtr<SpeechRecognition> mRecognition;
};

} 
} 
