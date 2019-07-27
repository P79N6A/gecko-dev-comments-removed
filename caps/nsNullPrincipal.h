










#ifndef nsNullPrincipal_h__
#define nsNullPrincipal_h__

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsIScriptSecurityManager.h"
#include "nsCOMPtr.h"
#include "nsIContentSecurityPolicy.h"

class nsIURI;

#define NS_NULLPRINCIPAL_CID \
{ 0xa0bd8b42, 0xf6bf, 0x4fb9, \
  { 0x93, 0x42, 0x90, 0xbf, 0xc9, 0xb7, 0xa1, 0xab } }
#define NS_NULLPRINCIPAL_CONTRACTID "@mozilla.org/nullprincipal;1"

#define NS_NULLPRINCIPAL_SCHEME "moz-nullprincipal"

class nsNullPrincipal final : public nsJSPrincipals
{
public:
  
  
  
  nsNullPrincipal();

  
  

  
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPRINCIPAL
  NS_DECL_NSISERIALIZABLE

  
  static already_AddRefed<nsNullPrincipal> CreateWithInheritedAttributes(nsIPrincipal *aInheritFrom);

  
  static already_AddRefed<nsNullPrincipal>
    Create(uint32_t aAppId = nsIScriptSecurityManager::NO_APP_ID,
           bool aInMozBrowser = false);

  nsresult Init(uint32_t aAppId = nsIScriptSecurityManager::NO_APP_ID,
                bool aInMozBrowser = false);

  virtual void GetScriptLocation(nsACString &aStr) override;

#ifdef DEBUG
  virtual void dumpImpl() override;
#endif 

 protected:
  virtual ~nsNullPrincipal();

  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
  uint32_t mAppId;
  bool mInMozBrowser;
};

#endif 
