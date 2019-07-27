





#ifndef mozilla_dom_MMICall_h
#define mozilla_dom_MMICall_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsJSUtils.h"
#include "nsWrapperCache.h"

struct JSContext;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class MMICall MOZ_FINAL : public nsISupports,
                          public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MMICall)

  MMICall(nsPIDOMWindow* aWindow, const nsAString& aServiceCode);

  nsPIDOMWindow*
  GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void
  NotifyResult(JS::Handle<JS::Value> aResult);

  
  already_AddRefed<Promise>
  GetResult(ErrorResult& aRv);

private:
  ~MMICall();

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsString mServiceCode;
  nsRefPtr<Promise> mPromise;
};

} 
} 

#endif 
