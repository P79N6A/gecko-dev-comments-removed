





#ifndef MediaElementAudioSourceNode_h_
#define MediaElementAudioSourceNode_h_

#include "MediaStreamAudioSourceNode.h"

namespace mozilla {
namespace dom {

class MediaElementAudioSourceNode final : public MediaStreamAudioSourceNode
{
public:
  MediaElementAudioSourceNode(AudioContext* aContext,
                              DOMMediaStream* aStream);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual const char* NodeType() const override
  {
    return "MediaElementAudioSourceNode";
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }
};

}
}

#endif
