




#ifndef mozilla_dom_Animation_h
#define mozilla_dom_Animation_h

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDocument.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/Nullable.h"

struct JSContext;

namespace mozilla {
namespace dom {

class Animation MOZ_FINAL : public nsWrapperCache
{
public:
  explicit Animation(nsIDocument* aDocument)
    : mDocument(aDocument)
  {
    SetIsDOMBinding();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(Animation)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(Animation)

  nsIDocument* GetParentObject() const { return mDocument; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void SetParentTime(Nullable<TimeDuration> aParentTime);

protected:
  virtual ~Animation() { }

  
  
  nsRefPtr<nsIDocument> mDocument;
  Nullable<TimeDuration> mParentTime;
};

} 
} 

#endif 
