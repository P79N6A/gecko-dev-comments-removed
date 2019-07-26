





#include "MediaElementAudioSourceNode.h"
#include "mozilla/dom/MediaElementAudioSourceNodeBinding.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {
namespace dom {

MediaElementAudioSourceNode::MediaElementAudioSourceNode(AudioContext* aContext,
                                                         DOMMediaStream* aStream)
  : MediaStreamAudioSourceNode(aContext, aStream)
{
}

JSObject*
MediaElementAudioSourceNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MediaElementAudioSourceNodeBinding::Wrap(aCx, aScope, this);
}

}
}
