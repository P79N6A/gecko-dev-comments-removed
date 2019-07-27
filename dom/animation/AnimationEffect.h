




#ifndef mozilla_dom_AnimationEffect_h
#define mozilla_dom_AnimationEffect_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/Animation.h"

struct JSContext;

namespace mozilla {
namespace dom {

class AnimationEffect MOZ_FINAL : public nsWrapperCache
{
public:
  explicit AnimationEffect(Animation* aAnimation)
    : mAnimation(aAnimation)
  {
    SetIsDOMBinding();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnimationEffect)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AnimationEffect)

  Animation* GetParentObject() const { return mAnimation; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  void GetName(nsString& aRetVal) const {
    aRetVal = mAnimation->Name();
  }

private:
  ~AnimationEffect() { }

  nsRefPtr<Animation> mAnimation;
};

} 
} 

#endif 
