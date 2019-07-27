





#ifndef OscillatorNode_h_
#define OscillatorNode_h_

#include "AudioNode.h"
#include "AudioParam.h"
#include "PeriodicWave.h"
#include "mozilla/dom/OscillatorNodeBinding.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {

class AudioContext;

class OscillatorNode final : public AudioNode,
                             public MainThreadMediaStreamListener
{
public:
  explicit OscillatorNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(OscillatorNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual void DestroyMediaStream() override
  {
    if (mStream) {
      mStream->RemoveMainThreadListener(this);
    }
    AudioNode::DestroyMediaStream();
  }
  virtual uint16_t NumberOfInputs() const final override
  {
    return 0;
  }

  OscillatorType Type() const
  {
    return mType;
  }
  void SetType(OscillatorType aType, ErrorResult& aRv)
  {
    if (aType == OscillatorType::Custom) {
      
      
      aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return;
    }
    mType = aType;
    SendTypeToStream();
  }

  AudioParam* Frequency() const
  {
    return mFrequency;
  }
  AudioParam* Detune() const
  {
    return mDetune;
  }

  void Start(double aWhen, ErrorResult& aRv);
  void Stop(double aWhen, ErrorResult& aRv);
  void SetPeriodicWave(PeriodicWave& aPeriodicWave)
  {
    mPeriodicWave = &aPeriodicWave;
    
    mType = OscillatorType::Custom;
    SendTypeToStream();
  }

  IMPL_EVENT_HANDLER(ended)

  virtual void NotifyMainThreadStateChanged() override;

  virtual const char* NodeType() const override
  {
    return "OscillatorNode";
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override;

protected:
  virtual ~OscillatorNode();

private:
  static void SendFrequencyToStream(AudioNode* aNode);
  static void SendDetuneToStream(AudioNode* aNode);
  void SendTypeToStream();
  void SendPeriodicWaveToStream();

private:
  OscillatorType mType;
  nsRefPtr<PeriodicWave> mPeriodicWave;
  nsRefPtr<AudioParam> mFrequency;
  nsRefPtr<AudioParam> mDetune;
  bool mStartCalled;
  bool mStopped;
};

}
}

#endif

