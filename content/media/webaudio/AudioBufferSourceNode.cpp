





#include "AudioBufferSourceNode.h"
#include "mozilla/dom/AudioBufferSourceNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(AudioBufferSourceNode, AudioSourceNode)

AudioBufferSourceNode::AudioBufferSourceNode(AudioContext* aContext)
  : AudioSourceNode(aContext)
{
}

JSObject*
AudioBufferSourceNode::WrapObject(JSContext* aCx, JSObject* aScope,
                                  bool* aTriedToWrap)
{
  return AudioBufferSourceNodeBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}

