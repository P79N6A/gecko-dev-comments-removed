










#ifndef nsNullPrincipal_h__
#define nsNullPrincipal_h__

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsIScriptSecurityManager.h"
#include "nsCOMPtr.h"
#include "nsIContentSecurityPolicy.h"

#include "mozilla/BasePrincipal.h"

class nsIURI;

#define NS_NULLPRINCIPAL_CID \
{ 0xe502ffb8, 0x5d95, 0x48e8, \
  { 0x82, 0x3c, 0x0d, 0x29, 0xd8, 0x3a, 0x59, 0x33 } }
#define NS_NULLPRINCIPAL_CONTRACTID "@mozilla.org/nullprincipal;1"

#define NS_NULLPRINCIPAL_SCHEME "moz-nullprincipal"

class nsNullPrincipal final : public mozilla::BasePrincipal
{
public:
  
  
  
  nsNullPrincipal() {}

  NS_DECL_NSISERIALIZABLE

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;
  NS_IMETHOD GetHashValue(uint32_t* aHashValue) override;
  NS_IMETHOD GetURI(nsIURI** aURI) override;
  NS_IMETHOD GetDomain(nsIURI** aDomain) override;
  NS_IMETHOD SetDomain(nsIURI* aDomain) override;
  NS_IMETHOD CheckMayLoad(nsIURI* uri, bool report, bool allowIfInheritsPrincipal) override;
  NS_IMETHOD GetIsNullPrincipal(bool* aIsNullPrincipal) override;
  NS_IMETHOD GetBaseDomain(nsACString& aBaseDomain) override;
  nsresult GetOriginInternal(nsACString& aOrigin) override;

  
  static already_AddRefed<nsNullPrincipal> CreateWithInheritedAttributes(nsIPrincipal *aInheritFrom);

  
  static already_AddRefed<nsNullPrincipal>
    Create(const mozilla::OriginAttributes& aOriginAttributes = mozilla::OriginAttributes());

  nsresult Init(const mozilla::OriginAttributes& aOriginAttributes = mozilla::OriginAttributes());

  virtual void GetScriptLocation(nsACString &aStr) override;

 protected:
  virtual ~nsNullPrincipal() {}

  bool SubsumesInternal(nsIPrincipal* aOther, DocumentDomainConsideration aConsideration) override
  {
    return aOther == this;
  }

  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
};

#endif 
