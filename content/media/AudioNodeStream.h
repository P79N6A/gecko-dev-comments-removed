




#ifndef MOZILLA_AUDIONODESTREAM_H_
#define MOZILLA_AUDIONODESTREAM_H_

#include "MediaStreamGraph.h"
#include "AudioChannelFormat.h"
#include "AudioNodeEngine.h"
#include "mozilla/dom/AudioParam.h"

#ifdef PR_LOGGING
#define LOG(type, msg) PR_LOG(gMediaStreamGraphLog, type, msg)
#else
#define LOG(type, msg)
#endif

namespace mozilla {

namespace dom {
struct ThreeDPoint;
}

class ThreadSharedFloatArrayBufferList;











class AudioNodeStream : public ProcessedMediaStream {
public:
  enum { AUDIO_TRACK = 1 };

  


  AudioNodeStream(AudioNodeEngine* aEngine,
                  MediaStreamGraph::AudioNodeStreamKind aKind)
    : ProcessedMediaStream(nullptr),
      mEngine(aEngine),
      mKind(aKind)
  {
  }
  ~AudioNodeStream();

  
  



  void SetStreamTimeParameter(uint32_t aIndex, MediaStream* aRelativeToStream,
                              double aStreamTime);
  void SetDoubleParameter(uint32_t aIndex, double aValue);
  void SetInt32Parameter(uint32_t aIndex, int32_t aValue);
  void SetTimelineParameter(uint32_t aIndex, const dom::AudioParamTimeline& aValue);
  void SetThreeDPointParameter(uint32_t aIndex, const dom::ThreeDPoint& aValue);
  void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer);

  virtual AudioNodeStream* AsAudioNodeStream() { return this; }

  
  void SetStreamTimeParameterImpl(uint32_t aIndex, MediaStream* aRelativeToStream,
                                  double aStreamTime);
  virtual void ProduceOutput(GraphTime aFrom, GraphTime aTo);
  TrackTicks GetCurrentPosition();

  
  AudioNodeEngine* Engine() { return mEngine; }

protected:
  void FinishOutput();

  StreamBuffer::Track* EnsureTrack();
  AudioChunk* ObtainInputBlock(AudioChunk* aTmpChunk);

  
  nsAutoPtr<AudioNodeEngine> mEngine;
  
  AudioChunk mLastChunk;
  
  MediaStreamGraph::AudioNodeStreamKind mKind;
};

}

#endif
