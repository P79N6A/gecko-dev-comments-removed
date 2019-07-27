




#ifndef mozilla_dom_AnimationEffect_h
#define mozilla_dom_AnimationEffect_h

#include "nsISupports.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {

class AnimationEffectReadOnly
  : public nsISupports
  , public nsWrapperCache
{
protected:
  virtual ~AnimationEffectReadOnly() { }

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AnimationEffectReadOnly)

  explicit AnimationEffectReadOnly(nsISupports* aParent)
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
