





#ifndef WaveShaperNode_h_
#define WaveShaperNode_h_

#include "AudioNode.h"
#include "AudioParam.h"
#include "mozilla/dom/WaveShaperNodeBinding.h"

namespace mozilla {
namespace dom {

class AudioContext;

class WaveShaperNode : public AudioNode
{
public:
  explicit WaveShaperNode(AudioContext *aContext);
  virtual ~WaveShaperNode();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(WaveShaperNode, AudioNode)

  virtual JSObject* WrapObject(JSContext *aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  JSObject* GetCurve(JSContext* aCx) const
  {
    return mCurve;
  }
  void SetCurve(const Nullable<Float32Array>& aData);

  OverSampleType Oversample() const
  {
    return mType;
  }
  void SetOversample(OverSampleType aType);

private:
  void ClearCurve();

private:
  JS::Heap<JSObject*> mCurve;
  OverSampleType mType;
};

}
}

#endif
