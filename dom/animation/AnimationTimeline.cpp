




#include "AnimationTimeline.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AnimationTimeline, mWindow)

NS_IMPL_CYCLE_COLLECTING_ADDREF(AnimationTimeline)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AnimationTimeline)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AnimationTimeline)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

} 
} 
