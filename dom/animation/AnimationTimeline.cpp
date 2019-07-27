





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

static PLDHashOperator
AppendAnimationToSequence(nsRefPtrHashKey<dom::Animation>* aKey,
                          void* aSequence)
{
  Animation* animation = aKey->GetKey();
  AnimationTimeline::AnimationSequence* sequence =
    static_cast<AnimationTimeline::AnimationSequence*>(aSequence);
  sequence->AppendElement(animation);

  return PL_DHASH_NEXT;
}

void
AnimationTimeline::GetAnimations(AnimationSequence& aAnimations)
{
  

  mAnimations.EnumerateEntries(AppendAnimationToSequence, &aAnimations);
}

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
