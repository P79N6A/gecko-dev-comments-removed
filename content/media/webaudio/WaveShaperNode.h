





#ifndef WaveShaperNode_h_
#define WaveShaperNode_h_

#include "AudioNode.h"
#include "mozilla/dom/WaveShaperNodeBinding.h"
#include "mozilla/dom/TypedArray.h"

namespace mozilla {
namespace dom {

class AudioContext;

class WaveShaperNode : public AudioNode
{
public:
  explicit WaveShaperNode(AudioContext *aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(WaveShaperNode, AudioNode)

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  void GetCurve(JSContext* aCx, JS::MutableHandle<JSObject*> aRetval) const
  {
    if (mCurve) {
      JS::ExposeObjectToActiveJS(mCurve);
    }
    aRetval.set(mCurve);
  }
  void SetCurve(const Nullable<Float32Array>& aData);

  OverSampleType Oversample() const
  {
    return mType;
  }
  void SetOversample(OverSampleType aType);

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    
    
    return AudioNode::SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual const char* NodeType() const
  {
    return "WaveShaperNode";
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  virtual ~WaveShaperNode();

private:
  void ClearCurve();

private:
  JS::Heap<JSObject*> mCurve;
  OverSampleType mType;
};

}
}

#endif
