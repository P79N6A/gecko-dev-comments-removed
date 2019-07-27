





#include "MediaElementAudioSourceNode.h"
#include "mozilla/dom/MediaElementAudioSourceNodeBinding.h"

namespace mozilla {
namespace dom {

MediaElementAudioSourceNode::MediaElementAudioSourceNode(AudioContext* aContext,
                                                         DOMMediaStream* aStream)
  : MediaStreamAudioSourceNode(aContext, aStream)
{
}

JSObject*
MediaElementAudioSourceNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MediaElementAudioSourceNodeBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
