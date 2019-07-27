




#ifndef RTCIDENTITYPROVIDER_H_
#define RTCIDENTITYPROVIDER_H_

#include "nsRefPtr.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsIGlobalObject.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/BindingDeclarations.h"

namespace mozilla {
namespace dom {

class RTCIdentityProvider;

class RTCIdentityProviderRegistrar MOZ_FINAL : public nsISupports,
                                               public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(RTCIdentityProviderRegistrar)

  explicit RTCIdentityProviderRegistrar(nsIGlobalObject* aGlobal);

  
  nsIGlobalObject* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx);

  
  void Register(RTCIdentityProvider& aIdp);
  already_AddRefed<RTCIdentityProvider> GetIdp();

  already_AddRefed<Promise>
  GenerateAssertion(const nsAString& aContents, const nsAString& aOrigin,
                    const Optional<nsAString>& aUsernameHint, ErrorResult& aRv);
  already_AddRefed<Promise>
  ValidateAssertion(const nsAString& assertion, const nsAString& origin,
                    ErrorResult& aRv);

private:
  ~RTCIdentityProviderRegistrar();

  nsCOMPtr<nsIGlobalObject> mGlobal;
  nsRefPtr<RTCIdentityProvider> mIdp;
};

} 
} 

#endif 
