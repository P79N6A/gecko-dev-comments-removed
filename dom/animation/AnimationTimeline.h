




#ifndef mozilla_dom_AnimationTimeline_h
#define mozilla_dom_AnimationTimeline_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "js/TypeDecls.h"
#include "nsIDocument.h"

struct JSContext;

namespace mozilla {

class TimeStamp;

namespace dom {

class AnimationTimeline MOZ_FINAL : public nsWrapperCache
{
public:
  AnimationTimeline(nsIDocument* aDocument)
    : mDocument(aDocument)
  {
    SetIsDOMBinding();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnimationTimeline)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AnimationTimeline)

  nsISupports* GetParentObject() const { return mDocument; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  Nullable<double> GetCurrentTime() const;
  mozilla::TimeStamp GetCurrentTimeStamp() const;

  Nullable<double> ToTimelineTime(const mozilla::TimeStamp& aTimeStamp) const;

protected:
  virtual ~AnimationTimeline() { }

  nsCOMPtr<nsIDocument> mDocument;
};

} 
} 

#endif 
