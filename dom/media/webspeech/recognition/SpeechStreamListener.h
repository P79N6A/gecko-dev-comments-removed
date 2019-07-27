





#ifndef mozilla_dom_SpeechStreamListener_h
#define mozilla_dom_SpeechStreamListener_h

#include "MediaStreamGraph.h"
#include "AudioSegment.h"

namespace mozilla {

class AudioSegment;

namespace dom {

class SpeechRecognition;

class SpeechStreamListener : public MediaStreamListener
{
public:
  explicit SpeechStreamListener(SpeechRecognition* aRecognition);
  ~SpeechStreamListener();

  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        StreamTime aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) MOZ_OVERRIDE;

  virtual void NotifyEvent(MediaStreamGraph* aGraph,
                           MediaStreamListener::MediaStreamGraphEvent event) MOZ_OVERRIDE;

private:
  template<typename SampleFormatType>
  void ConvertAndDispatchAudioChunk(int aDuration, float aVolume, SampleFormatType* aData, TrackRate aTrackRate);
  nsRefPtr<SpeechRecognition> mRecognition;
};

} 
} 

#endif
