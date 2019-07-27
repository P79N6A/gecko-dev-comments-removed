




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
                  TrackRate aSampleRate)
    : ProcessedMediaStream(nullptr),
      mEngine(aEngine),
      mSampleRate(aSampleRate),
      mKind(aKind),
      mNumberOfInputChannels(2),
      mMarkAsFinishedAfterThisBlock(false),
      mAudioParamStream(false),
      mPassThrough(false)
  {
    MOZ_ASSERT(NS_IsMainThread());
    mChannelCountMode = ChannelCountMode::Max;
    mChannelInterpretation = ChannelInterpretation::Speakers;
    
    mHasCurrentData = true;
    MOZ_COUNT_CTOR(AudioNodeStream);
  }

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

  virtual AudioNodeStream* AsAudioNodeStream() MOZ_OVERRIDE { return this; }

  
  void SetStreamTimeParameterImpl(uint32_t aIndex, MediaStream* aRelativeToStream,
                                  double aStreamTime);
  void SetChannelMixingParametersImpl(uint32_t aNumberOfChannels,
                                      ChannelCountMode aChannelCountMoe,
                                      ChannelInterpretation aChannelInterpretation);
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags) MOZ_OVERRIDE;
  




  void ProduceOutputBeforeInput(GraphTime aFrom);
  TrackTicks GetCurrentPosition();
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

  



  double TimeFromDestinationTime(AudioNodeStream* aDestination,
                                 double aSeconds);
  



  TrackTicks TicksFromDestinationTime(MediaStream* aDestination,
                                      double aSeconds);
  



  double DestinationTimeFromTicks(AudioNodeStream* aDestination,
                                  TrackTicks aPosition);

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

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
  
  MediaStreamGraph::AudioNodeStreamKind mKind;
  
  uint32_t mNumberOfInputChannels;
  
  ChannelCountMode mChannelCountMode;
  ChannelInterpretation mChannelInterpretation;
  
  
  bool mMarkAsFinishedAfterThisBlock;
  
  bool mAudioParamStream;
  
  bool mPassThrough;
};

}

#endif 
