




#ifndef mozilla_dom_AnimationEffect_h
#define mozilla_dom_AnimationEffect_h

#include "nsISupports.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {

class AnimationEffectReadonly
  : public nsISupports
  , public nsWrapperCache
{
protected:
  virtual ~AnimationEffectReadonly() { }

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AnimationEffectReadonly)

  explicit AnimationEffectReadonly(nsISupports* aParent)
    : mParent(aParent)
  {
  }

  nsISupports* GetParentObject() const { return mParent; }

protected:
  nsCOMPtr<nsISupports> mParent;
};

} 
} 

#endif 
