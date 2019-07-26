





#ifndef ConvolverNode_h_
#define ConvolverNode_h_

#include "AudioNode.h"
#include "AudioBuffer.h"

namespace mozilla {
namespace dom {

class ConvolverNode : public AudioNode
{
public:
  explicit ConvolverNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ConvolverNode, AudioNode);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  AudioBuffer* GetBuffer(JSContext* aCx) const
  {
    return mBuffer;
  }

  void SetBuffer(JSContext* aCx, AudioBuffer* aBufferi, ErrorResult& aRv);

  bool Normalize() const
  {
    return mNormalize;
  }

  void SetNormalize(bool aNormal);

private:
  nsRefPtr<AudioBuffer> mBuffer;
  bool mNormalize;
};


} 
} 

#endif

