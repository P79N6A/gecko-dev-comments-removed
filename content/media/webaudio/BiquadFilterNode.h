





#ifndef BiquadFilterNode_h_
#define BiquadFilterNode_h_

#include "AudioNode.h"
#include "AudioParam.h"
#include "mozilla/dom/BiquadFilterNodeBinding.h"

namespace mozilla {
namespace dom {

class AudioContext;

class BiquadFilterNode : public AudioNode
{
public:
  explicit BiquadFilterNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BiquadFilterNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
  }

  BiquadFilterType Type() const
  {
    return mType;
  }
  void SetType(BiquadFilterType aType);

  AudioParam* Frequency() const
  {
    return mFrequency;
  }

  AudioParam* Detune() const
  {
    return mDetune;
  }

  AudioParam* Q() const
  {
    return mQ;
  }

  AudioParam* Gain() const
  {
    return mGain;
  }

private:
  static void SendFrequencyToStream(AudioNode* aNode);
  static void SendDetuneToStream(AudioNode* aNode);
  static void SendQToStream(AudioNode* aNode);
  static void SendGainToStream(AudioNode* aNode);

private:
  BiquadFilterType mType;
  nsRefPtr<AudioParam> mFrequency;
  nsRefPtr<AudioParam> mDetune;
  nsRefPtr<AudioParam> mQ;
  nsRefPtr<AudioParam> mGain;
};

}
}

#endif

