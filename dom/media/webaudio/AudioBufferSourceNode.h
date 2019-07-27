





#ifndef AudioBufferSourceNode_h_
#define AudioBufferSourceNode_h_

#include "AudioNode.h"
#include "AudioBuffer.h"

namespace mozilla {
namespace dom {

class AudioParam;

class AudioBufferSourceNode : public AudioNode,
                              public MainThreadMediaStreamListener
{
public:
  explicit AudioBufferSourceNode(AudioContext* aContext);

  virtual void DestroyMediaStream() MOZ_OVERRIDE
  {
    if (mStream) {
      mStream->RemoveMainThreadListener(this);
    }
    AudioNode::DestroyMediaStream();
  }
  virtual uint16_t NumberOfInputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }
  virtual AudioBufferSourceNode* AsAudioBufferSourceNode() MOZ_OVERRIDE
  {
    return this;
  }
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioBufferSourceNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void Start(double aWhen, double aOffset,
             const Optional<double>& aDuration, ErrorResult& aRv);
  void Stop(double aWhen, ErrorResult& aRv);

  AudioBuffer* GetBuffer(JSContext* aCx) const
  {
    return mBuffer;
  }
  void SetBuffer(JSContext* aCx, AudioBuffer* aBuffer)
  {
    mBuffer = aBuffer;
    SendBufferParameterToStream(aCx);
    SendLoopParametersToStream();
  }
  AudioParam* PlaybackRate() const
  {
    return mPlaybackRate;
  }
  bool Loop() const
  {
    return mLoop;
  }
  void SetLoop(bool aLoop)
  {
    mLoop = aLoop;
    SendLoopParametersToStream();
  }
  double LoopStart() const
  {
    return mLoopStart;
  }
  void SetLoopStart(double aStart)
  {
    mLoopStart = aStart;
    SendLoopParametersToStream();
  }
  double LoopEnd() const
  {
    return mLoopEnd;
  }
  void SetLoopEnd(double aEnd)
  {
    mLoopEnd = aEnd;
    SendLoopParametersToStream();
  }
  void SendDopplerShiftToStream(double aDopplerShift);

  IMPL_EVENT_HANDLER(ended)

  virtual void NotifyMainThreadStateChanged() MOZ_OVERRIDE;

  virtual const char* NodeType() const
  {
    return "AudioBufferSourceNode";
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

protected:
  virtual ~AudioBufferSourceNode();

private:
  friend class AudioBufferSourceNodeEngine;
  
  
  
  
  enum EngineParameters {
    SAMPLE_RATE,
    START,
    STOP,
    
    
    BUFFERSTART,
    
    
    BUFFEREND,
    LOOP,
    LOOPSTART,
    LOOPEND,
    PLAYBACKRATE,
    DOPPLERSHIFT
  };

  void SendLoopParametersToStream();
  void SendBufferParameterToStream(JSContext* aCx);
  void SendOffsetAndDurationParametersToStream(AudioNodeStream* aStream);
  static void SendPlaybackRateToStream(AudioNode* aNode);

private:
  double mLoopStart;
  double mLoopEnd;
  double mOffset;
  double mDuration;
  nsRefPtr<AudioBuffer> mBuffer;
  nsRefPtr<AudioParam> mPlaybackRate;
  bool mLoop;
  bool mStartCalled;
  bool mStopped;
};

}
}

#endif

