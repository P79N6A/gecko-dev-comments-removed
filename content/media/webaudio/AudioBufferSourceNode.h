





#ifndef AudioBufferSourceNode_h_
#define AudioBufferSourceNode_h_

#include "AudioSourceNode.h"
#include "AudioBuffer.h"
#include "mozilla/dom/BindingUtils.h"

namespace mozilla {
namespace dom {

class AudioBufferSourceNode : public AudioSourceNode,
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
    AudioSourceNode::DestroyMediaStream();
  }
  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
  }

  void JSBindingFinalized()
  {
    
    
    if (!mStartCalled) {
      SetProduceOwnOutput(false);
    }
    AudioSourceNode::JSBindingFinalized();
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioBufferSourceNode, AudioSourceNode)

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  void Start(JSContext* aCx, double aWhen, double aOffset,
             const Optional<double>& aDuration, ErrorResult& aRv);
  void Stop(double aWhen, ErrorResult& aRv);

  AudioBuffer* GetBuffer() const
  {
    return mBuffer;
  }
  void SetBuffer(AudioBuffer* aBuffer)
  {
    mBuffer = aBuffer;
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

  virtual void NotifyMainThreadStateChanged() MOZ_OVERRIDE;

private:
  nsRefPtr<AudioBuffer> mBuffer;
  double mLoopStart;
  double mLoopEnd;
  bool mLoop;
  bool mStartCalled;
};

}
}

#endif

