





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

namespace {
  struct AddAnimationParams {
    AnimationTimeline::AnimationSequence& mSequence;
#ifdef DEBUG
    
    AnimationTimeline* mTimeline;
#endif
  };
}

static PLDHashOperator
AppendAnimationToSequence(nsRefPtrHashKey<dom::Animation>* aKey,
                          void* aParams)
{
  Animation* animation = aKey->GetKey();
  AddAnimationParams* params = static_cast<AddAnimationParams*>(aParams);

  MOZ_ASSERT(animation->IsRelevant(),
             "Animations registered with a timeline should be relevant");
  MOZ_ASSERT(animation->GetTimeline() == params->mTimeline,
             "Animation should refer to this timeline");

  params->mSequence.AppendElement(animation);

  return PL_DHASH_NEXT;
}

void
AnimationTimeline::GetAnimations(AnimationSequence& aAnimations)
{
  

#ifdef DEBUG
  AddAnimationParams params{ aAnimations, this };
#else
  AddAnimationParams params{ aAnimations };
#endif
  mAnimations.EnumerateEntries(AppendAnimationToSequence, &params);
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
