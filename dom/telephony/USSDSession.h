





#ifndef mozilla_dom_USSDSession_h
#define mozilla_dom_USSDSession_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Promise.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsITelephonyService.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

struct JSContext;

namespace mozilla {
namespace dom {

class USSDSession MOZ_FINAL : public nsISupports,
                              public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(USSDSession)

  USSDSession(nsPIDOMWindow* aWindow, nsITelephonyService* aService,
              uint32_t aServiceId);

  nsPIDOMWindow*
  GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  
  static already_AddRefed<USSDSession>
  Constructor(const GlobalObject& aGlobal, uint32_t aServiceId,
              ErrorResult& aRv);

  already_AddRefed<Promise>
  Send(const nsAString& aUssd, ErrorResult& aRv);

  already_AddRefed<Promise>
  Cancel(ErrorResult& aRv);

private:
  ~USSDSession();

  already_AddRefed<Promise>
  CreatePromise(ErrorResult& aRv);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsCOMPtr<nsITelephonyService> mService;
  uint32_t mServiceId;
};

} 
} 

#endif 
