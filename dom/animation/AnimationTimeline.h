




#ifndef mozilla_dom_AnimationTimeline_h
#define mozilla_dom_AnimationTimeline_h

#include "nsISupports.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/AnimationUtils.h"
#include "mozilla/Attributes.h"
#include "nsIGlobalObject.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class AnimationTimeline
  : public nsISupports
  , public nsWrapperCache
{
public:
  explicit AnimationTimeline(nsIGlobalObject* aWindow)
    : mWindow(aWindow)
  {
    MOZ_ASSERT(mWindow);
  }

protected:
  virtual ~AnimationTimeline() { }

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AnimationTimeline)

  nsIGlobalObject* GetParentObject() const { return mWindow; }

  
  virtual Nullable<TimeDuration> GetCurrentTime() const = 0;

  
  
  Nullable<double> GetCurrentTimeAsDouble() const {
    return AnimationUtils::TimeDurationToDouble(GetCurrentTime());
  }

protected:
  nsCOMPtr<nsIGlobalObject> mWindow;
};

} 
} 

#endif 
