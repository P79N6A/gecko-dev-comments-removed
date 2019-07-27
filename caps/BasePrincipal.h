





#ifndef mozilla_BasePrincipal_h
#define mozilla_BasePrincipal_h

#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsJSPrincipals.h"

namespace mozilla {








class BasePrincipal : public nsJSPrincipals
{
public:
  BasePrincipal()
    : mAppId(nsIScriptSecurityManager::NO_APP_ID)
    , mIsInBrowserElement(false)
  {}

  NS_IMETHOD GetCsp(nsIContentSecurityPolicy** aCsp) override;
  NS_IMETHOD SetCsp(nsIContentSecurityPolicy* aCsp) override;
  NS_IMETHOD GetIsNullPrincipal(bool* aIsNullPrincipal) override;
  NS_IMETHOD GetJarPrefix(nsACString& aJarPrefix) final;
  NS_IMETHOD GetAppStatus(uint16_t* aAppStatus) final;
  NS_IMETHOD GetAppId(uint32_t* aAppStatus) final;
  NS_IMETHOD GetIsInBrowserElement(bool* aIsInBrowserElement) final;
  NS_IMETHOD GetUnknownAppId(bool* aUnknownAppId) final;

  virtual bool IsOnCSSUnprefixingWhitelist() override { return false; }

protected:
  virtual ~BasePrincipal() {}

  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
  uint32_t mAppId;
  bool mIsInBrowserElement;
};

} 

#endif 
