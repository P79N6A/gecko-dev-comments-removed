





#include "AnimationTimeline.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AnimationTimeline, mWindow, mAnimations)

NS_IMPL_CYCLE_COLLECTING_ADDREF(AnimationTimeline)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AnimationTimeline)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AnimationTimeline)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

void
AnimationTimeline::AddAnimation(Animation& aAnimation)
{
  mAnimations.PutEntry(&aAnimation);
}

void
AnimationTimeline::RemoveAnimation(Animation& aAnimation)
{
  mAnimations.RemoveEntry(&aAnimation);
}

} 
} 
