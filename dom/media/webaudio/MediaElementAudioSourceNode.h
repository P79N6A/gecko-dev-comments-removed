





#ifndef MediaElementAudioSourceNode_h_
#define MediaElementAudioSourceNode_h_

#include "MediaStreamAudioSourceNode.h"

namespace mozilla {
namespace dom {

class MediaElementAudioSourceNode : public MediaStreamAudioSourceNode
{
public:
  MediaElementAudioSourceNode(AudioContext* aContext,
                              DOMMediaStream* aStream);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual const char* NodeType() const
  {
    return "MediaElementAudioSourceNode";
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }
};

}
}

#endif
