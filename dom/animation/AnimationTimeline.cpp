





#include "AnimationTimeline.h"
#include "mozilla/AnimationComparator.h"

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

  
  
  
  
  
  
  Element* target;
  nsCSSPseudoElements::Type pseudoType;
  animation->GetEffect()->GetTarget(target, pseudoType);
  if (pseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    return PL_DHASH_NEXT;
  }

  params->mSequence.AppendElement(animation);

  return PL_DHASH_NEXT;
}

void
AnimationTimeline::GetAnimations(AnimationSequence& aAnimations)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mWindow);
  if (mWindow) {
    nsIDocument* doc = window->GetDoc();
    if (doc) {
      doc->FlushPendingNotifications(Flush_Style);
    }
  }

#ifdef DEBUG
  AddAnimationParams params{ aAnimations, this };
#else
  AddAnimationParams params{ aAnimations };
#endif
  mAnimations.EnumerateEntries(AppendAnimationToSequence, &params);

  
  aAnimations.Sort(AnimationPtrComparator<nsRefPtr<Animation>>());
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
