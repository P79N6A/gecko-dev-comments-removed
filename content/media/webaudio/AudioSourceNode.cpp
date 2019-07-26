





#include "AudioSourceNode.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(AudioSourceNode, AudioNode)

AudioSourceNode::AudioSourceNode(AudioContext* aContext)
  : AudioNode(aContext)
{
}

}
}

