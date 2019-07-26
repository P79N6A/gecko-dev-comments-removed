





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

class OscillatorNode : public AudioNode,
                       public MainThreadMediaStreamListener
{
public:
  explicit OscillatorNode(AudioContext* aContext);
  virtual ~OscillatorNode();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(OscillatorNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

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

  OscillatorType Type() const
  {
    return mType;
  }
  void SetType(OscillatorType aType, ErrorResult& aRv)
  {
    if (!Preferences::GetBool("media.webaudio.legacy.OscillatorNode")) {
      
      
      switch (aType) {
      case OscillatorType::_0:
      case OscillatorType::_1:
      case OscillatorType::_2:
      case OscillatorType::_3:
      case OscillatorType::_4:
        
        return;
      default:
        
        break;
      }
    }

    
    switch (aType) {
    case OscillatorType::_0: aType = OscillatorType::Sine; break;
    case OscillatorType::_1: aType = OscillatorType::Square; break;
    case OscillatorType::_2: aType = OscillatorType::Sawtooth; break;
    case OscillatorType::_3: aType = OscillatorType::Triangle; break;
    case OscillatorType::_4: aType = OscillatorType::Custom; break;
    default:
      
      break;
    }
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
  void NoteOn(double aWhen, ErrorResult& aRv)
  {
    Start(aWhen, aRv);
  }
  void Stop(double aWhen, ErrorResult& aRv);
  void NoteOff(double aWhen, ErrorResult& aRv)
  {
    Stop(aWhen, aRv);
  }
  void SetPeriodicWave(PeriodicWave& aPeriodicWave)
  {
    mPeriodicWave = &aPeriodicWave;
    
    mType = OscillatorType::Custom;
    SendTypeToStream();
  }

  IMPL_EVENT_HANDLER(ended)

  virtual void NotifyMainThreadStateChanged() MOZ_OVERRIDE;

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

