





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
};

}
}

#endif
