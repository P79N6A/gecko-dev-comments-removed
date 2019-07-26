




#ifndef MOZILLA_AUDIONODESTREAM_H_
#define MOZILLA_AUDIONODESTREAM_H_

#include "MediaStreamGraph.h"
#include "mozilla/dom/AudioNodeBinding.h"
#include "AudioSegment.h"

#ifdef PR_LOGGING
#define LOG(type, msg) PR_LOG(gMediaStreamGraphLog, type, msg)
#else
#define LOG(type, msg)
#endif

namespace mozilla {

namespace dom {
struct ThreeDPoint;
class AudioParamTimeline;
}

class ThreadSharedFloatArrayBufferList;
class AudioNodeEngine;











class AudioNodeStream : public ProcessedMediaStream {
public:
  enum { AUDIO_TRACK = 1 };

  typedef nsAutoTArray<AudioChunk, 1> OutputChunks;

  


  AudioNodeStream(AudioNodeEngine* aEngine,
                  MediaStreamGraph::AudioNodeStreamKind aKind,
                  TrackRate aSampleRate)
    : ProcessedMediaStream(nullptr),
      mEngine(aEngine),
      mSampleRate(aSampleRate),
      mKind(aKind),
      mNumberOfInputChannels(2),
      mMarkAsFinishedAfterThisBlock(false),
      mAudioParamStream(false)
  {
    MOZ_ASSERT(NS_IsMainThread());
    mChannelCountMode = dom::ChannelCountMode::Max;
    mChannelInterpretation = dom::ChannelInterpretation::Speakers;
    
    mHasCurrentData = true;
    MOZ_COUNT_CTOR(AudioNodeStream);
  }
  ~AudioNodeStream();

  
  



  void SetStreamTimeParameter(uint32_t aIndex, MediaStream* aRelativeToStream,
                              double aStreamTime);
  void SetDoubleParameter(uint32_t aIndex, double aValue);
  void SetInt32Parameter(uint32_t aIndex, int32_t aValue);
  void SetTimelineParameter(uint32_t aIndex, const dom::AudioParamTimeline& aValue);
  void SetThreeDPointParameter(uint32_t aIndex, const dom::ThreeDPoint& aValue);
  void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer);
  
  void SetRawArrayData(nsTArray<float>& aData);
  void SetChannelMixingParameters(uint32_t aNumberOfChannels,
                                  dom::ChannelCountMode aChannelCountMoe,
                                  dom::ChannelInterpretation aChannelInterpretation);
  void SetAudioParamHelperStream()
  {
    MOZ_ASSERT(!mAudioParamStream, "Can only do this once");
    mAudioParamStream = true;
  }

  virtual AudioNodeStream* AsAudioNodeStream() { return this; }

  
  void SetStreamTimeParameterImpl(uint32_t aIndex, MediaStream* aRelativeToStream,
                                  double aStreamTime);
  void SetChannelMixingParametersImpl(uint32_t aNumberOfChannels,
                                      dom::ChannelCountMode aChannelCountMoe,
                                      dom::ChannelInterpretation aChannelInterpretation);
  virtual void ProduceOutput(GraphTime aFrom, GraphTime aTo);
  TrackTicks GetCurrentPosition();
  bool AllInputsFinished() const;
  bool IsAudioParamStream() const
  {
    return mAudioParamStream;
  }
  const OutputChunks& LastChunks() const
  {
    return mLastChunks;
  }
  virtual bool MainThreadNeedsUpdates() const MOZ_OVERRIDE
  {
    
    return (mKind == MediaStreamGraph::SOURCE_STREAM && mFinished) ||
           mKind == MediaStreamGraph::EXTERNAL_STREAM;
  }
  virtual bool IsIntrinsicallyConsumed() const MOZ_OVERRIDE
  {
    return true;
  }

  
  AudioNodeEngine* Engine() { return mEngine; }
  TrackRate SampleRate() const { return mSampleRate; }

protected:
  void AdvanceOutputSegment();
  void FinishOutput();
  void AccumulateInputChunk(uint32_t aInputIndex, const AudioChunk& aChunk,
                            AudioChunk* aBlock,
                            nsTArray<float>* aDownmixBuffer);
  void UpMixDownMixChunk(const AudioChunk* aChunk, uint32_t aOutputChannelCount,
                         nsTArray<const void*>& aOutputChannels,
                         nsTArray<float>& aDownmixBuffer);

  uint32_t ComputeFinalOuputChannelCount(uint32_t aInputChannelCount);
  void ObtainInputBlock(AudioChunk& aTmpChunk, uint32_t aPortIndex);

  
  nsAutoPtr<AudioNodeEngine> mEngine;
  
  OutputChunks mLastChunks;
  
  const TrackRate mSampleRate;
  
  MediaStreamGraph::AudioNodeStreamKind mKind;
  
  uint32_t mNumberOfInputChannels;
  
  dom::ChannelCountMode mChannelCountMode;
  dom::ChannelInterpretation mChannelInterpretation;
  
  
  bool mMarkAsFinishedAfterThisBlock;
  
  bool mAudioParamStream;
};

}

#endif 
