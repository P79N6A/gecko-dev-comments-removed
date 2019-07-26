





#include "AudioNode.h"
#include "AudioContext.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(AudioNode, mContext)
NS_IMPL_CYCLE_COLLECTING_ADDREF(AudioNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AudioNode)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AudioNode)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

AudioNode::AudioNode(AudioContext* aContext)
  : mContext(aContext)
{
  MOZ_ASSERT(aContext);
  SetIsDOMBinding();
}

}
}

