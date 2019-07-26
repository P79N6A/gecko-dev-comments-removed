





#ifndef MediaElementAudioSourceNode_h_
#define MediaElementAudioSourceNode_h_

#include "MediaStreamAudioSourceNode.h"

namespace mozilla {
namespace dom {

class HTMLMediaElement;

class MediaElementAudioSourceNode : public MediaStreamAudioSourceNode
{
public:
  MediaElementAudioSourceNode(AudioContext* aContext,
                              DOMMediaStream* aStream);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
};

}
}

#endif
