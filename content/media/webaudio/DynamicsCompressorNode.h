





#ifndef DynamicsCompressorNode_h_
#define DynamicsCompressorNode_h_

#include "AudioNode.h"
#include "AudioParam.h"

namespace mozilla {
namespace dom {

class AudioContext;

class DynamicsCompressorNode : public AudioNode
{
public:
  explicit DynamicsCompressorNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DynamicsCompressorNode, AudioNode)

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  AudioParam* Threshold() const
  {
    return mThreshold;
  }

  AudioParam* Knee() const
  {
    return mKnee;
  }

  AudioParam* Ratio() const
  {
    return mRatio;
  }

  AudioParam* Reduction() const
  {
    return mReduction;
  }

  AudioParam* Attack() const
  {
    return mAttack;
  }

  
  AudioParam* GetRelease() const
  {
    return mRelease;
  }

private:
  nsRefPtr<AudioParam> mThreshold;
  nsRefPtr<AudioParam> mKnee;
  nsRefPtr<AudioParam> mRatio;
  nsRefPtr<AudioParam> mReduction;
  nsRefPtr<AudioParam> mAttack;
  nsRefPtr<AudioParam> mRelease;
};

}
}

#endif

