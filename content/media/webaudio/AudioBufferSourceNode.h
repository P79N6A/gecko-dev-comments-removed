





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
  virtual ~AudioBufferSourceNode();

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

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void Start(double aWhen, double aOffset,
             const Optional<double>& aDuration, ErrorResult& aRv);
  void NoteOn(double aWhen, ErrorResult& aRv)
  {
    Start(aWhen, 0.0, Optional<double>(), aRv);
  }
  void NoteGrainOn(double aWhen, double aOffset,
                   double aDuration, ErrorResult& aRv)
  {
    Optional<double> duration;
    duration.Construct(aDuration);
    Start(aWhen, aOffset, duration, aRv);
  }
  void Stop(double aWhen, ErrorResult& aRv);
  void NoteOff(double aWhen, ErrorResult& aRv)
  {
    Stop(aWhen, aRv);
  }

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

private:
  friend class AudioBufferSourceNodeEngine;
  
  
  
  enum EngineParameters {
    SAMPLE_RATE,
    START,
    STOP,
    OFFSET,
    DURATION,
    LOOP,
    LOOPSTART,
    LOOPEND,
    PLAYBACKRATE,
    DOPPLERSHIFT
  };

  void SendLoopParametersToStream();
  void SendBufferParameterToStream(JSContext* aCx);
  void SendOffsetAndDurationParametersToStream(AudioNodeStream* aStream,
                                               double aOffset,
                                               double aDuration);
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

