




#ifndef MOZILLA_AUDIONODESTREAM_H_
#define MOZILLA_AUDIONODESTREAM_H_

#include "MediaStreamGraph.h"
#include "mozilla/dom/AudioNodeBinding.h"
#include "AudioSegment.h"

namespace mozilla {

namespace dom {
struct ThreeDPoint;
class AudioParamTimeline;
class AudioContext;
}

class ThreadSharedFloatArrayBufferList;
class AudioNodeEngine;











class AudioNodeStream : public ProcessedMediaStream {
  typedef dom::ChannelCountMode ChannelCountMode;
  typedef dom::ChannelInterpretation ChannelInterpretation;

public:
  typedef mozilla::dom::AudioContext AudioContext;

  enum { AUDIO_TRACK = 1 };

  typedef nsAutoTArray<AudioChunk, 1> OutputChunks;

  


  AudioNodeStream(AudioNodeEngine* aEngine,
                  MediaStreamGraph::AudioNodeStreamKind aKind,
                  TrackRate aSampleRate,
                  AudioContext::AudioContextId aContextId);

protected:
  ~AudioNodeStream();

public:
  
  



  void SetStreamTimeParameter(uint32_t aIndex, AudioContext* aContext,
                              double aStreamTime);
  void SetDoubleParameter(uint32_t aIndex, double aValue);
  void SetInt32Parameter(uint32_t aIndex, int32_t aValue);
  void SetTimelineParameter(uint32_t aIndex, const dom::AudioParamTimeline& aValue);
  void SetThreeDPointParameter(uint32_t aIndex, const dom::ThreeDPoint& aValue);
  void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList>&& aBuffer);
  
  void SetRawArrayData(nsTArray<float>& aData);
  void SetChannelMixingParameters(uint32_t aNumberOfChannels,
                                  ChannelCountMode aChannelCountMoe,
                                  ChannelInterpretation aChannelInterpretation);
  void SetPassThrough(bool aPassThrough);
  ChannelInterpretation GetChannelInterpretation()
  {
    return mChannelInterpretation;
  }

  void SetAudioParamHelperStream()
  {
    MOZ_ASSERT(!mAudioParamStream, "Can only do this once");
    mAudioParamStream = true;
  }

  virtual AudioNodeStream* AsAudioNodeStream() override { return this; }

  
  void SetStreamTimeParameterImpl(uint32_t aIndex, MediaStream* aRelativeToStream,
                                  double aStreamTime);
  void SetChannelMixingParametersImpl(uint32_t aNumberOfChannels,
                                      ChannelCountMode aChannelCountMoe,
                                      ChannelInterpretation aChannelInterpretation);
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags) override;
  




  void ProduceOutputBeforeInput(GraphTime aFrom);
  StreamTime GetCurrentPosition();
  bool IsAudioParamStream() const
  {
    return mAudioParamStream;
  }

  const OutputChunks& LastChunks() const
  {
    return mLastChunks;
  }
  virtual bool MainThreadNeedsUpdates() const override
  {
    
    return (mKind == MediaStreamGraph::SOURCE_STREAM && mFinished) ||
           mKind == MediaStreamGraph::EXTERNAL_STREAM;
  }
  virtual bool IsIntrinsicallyConsumed() const override
  {
    return true;
  }

  
  AudioNodeEngine* Engine() { return mEngine; }
  TrackRate SampleRate() const { return mSampleRate; }
  AudioContext::AudioContextId AudioContextId() const override { return mAudioContextId; }

  



  double FractionalTicksFromDestinationTime(AudioNodeStream* aDestination,
                                            double aSeconds);
  



  StreamTime TicksFromDestinationTime(MediaStream* aDestination,
                                      double aSeconds);
  



  double DestinationTimeFromTicks(AudioNodeStream* aDestination,
                                  StreamTime aPosition);

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override;
  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override;

  void SizeOfAudioNodesIncludingThis(MallocSizeOf aMallocSizeOf,
                                     AudioNodeSizes& aUsage) const;


protected:
  void AdvanceOutputSegment();
  void FinishOutput();
  void AccumulateInputChunk(uint32_t aInputIndex, const AudioChunk& aChunk,
                            AudioChunk* aBlock,
                            nsTArray<float>* aDownmixBuffer);
  void UpMixDownMixChunk(const AudioChunk* aChunk, uint32_t aOutputChannelCount,
                         nsTArray<const void*>& aOutputChannels,
                         nsTArray<float>& aDownmixBuffer);

  uint32_t ComputedNumberOfChannels(uint32_t aInputChannelCount);
  void ObtainInputBlock(AudioChunk& aTmpChunk, uint32_t aPortIndex);

  
  nsAutoPtr<AudioNodeEngine> mEngine;
  
  OutputChunks mLastChunks;
  
  const TrackRate mSampleRate;
  
  
  const AudioContext::AudioContextId mAudioContextId;
  
  const MediaStreamGraph::AudioNodeStreamKind mKind;
  
  uint32_t mNumberOfInputChannels;
  
  ChannelCountMode mChannelCountMode;
  ChannelInterpretation mChannelInterpretation;
  
  
  bool mMarkAsFinishedAfterThisBlock;
  
  bool mAudioParamStream;
  
  bool mPassThrough;
};

}

#endif 
