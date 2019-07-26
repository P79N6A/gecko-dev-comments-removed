





#ifndef AudioBufferSourceNode_h_
#define AudioBufferSourceNode_h_

#include "AudioNode.h"
#include "AudioBuffer.h"
#include "AudioParam.h"
#include "mozilla/dom/BindingUtils.h"

namespace mozilla {
namespace dom {

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
  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
  }
  virtual uint32_t NumberOfInputs() const MOZ_FINAL MOZ_OVERRIDE
  {
    return 0;
  }
  virtual AudioBufferSourceNode* AsAudioBufferSourceNode() MOZ_OVERRIDE
  {
    return this;
  }

  void UnregisterPannerNode() {
    mPannerNode = nullptr;
  }

  void RegisterPannerNode(PannerNode* aPannerNode) {
    mPannerNode = aPannerNode;
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioBufferSourceNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  void Start(JSContext* aCx, double aWhen, double aOffset,
             const Optional<double>& aDuration, ErrorResult& aRv);
  void NoteOn(JSContext* aCx, double aWhen, ErrorResult& aRv)
  {
    Start(aCx, aWhen, 0.0, Optional<double>(), aRv);
  }
  void NoteGrainOn(JSContext* aCx, double aWhen, double aOffset,
                   double aDuration, ErrorResult& aRv)
  {
    Optional<double> duration;
    duration.Construct(aDuration);
    Start(aCx, aWhen, aOffset, duration, aRv);
  }
  void Stop(double aWhen, ErrorResult& aRv);
  void NoteOff(double aWhen, ErrorResult& aRv)
  {
    Stop(aWhen, aRv);
  }

  AudioBuffer* GetBuffer() const
  {
    return mBuffer;
  }
  void SetBuffer(AudioBuffer* aBuffer)
  {
    mBuffer = aBuffer;
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
  }
  double LoopStart() const
  {
    return mLoopStart;
  }
  void SetLoopStart(double aStart)
  {
    mLoopStart = aStart;
  }
  double LoopEnd() const
  {
    return mLoopEnd;
  }
  void SetLoopEnd(double aEnd)
  {
    mLoopEnd = aEnd;
  }
  void SendDopplerShiftToStream(double aDopplerShift);

  virtual void NotifyMainThreadStateChanged() MOZ_OVERRIDE;

private:
  static void SendPlaybackRateToStream(AudioNode* aNode);
  double mLoopStart;
  double mLoopEnd;
  nsRefPtr<AudioBuffer> mBuffer;
  nsRefPtr<AudioParam> mPlaybackRate;
  PannerNode* mPannerNode;
  SelfReference<AudioBufferSourceNode> mPlayingRef; 
  bool mLoop;
  bool mStartCalled;
};

}
}

#endif

