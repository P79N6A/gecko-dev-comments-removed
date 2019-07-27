







#ifndef nsSystemPrincipal_h__
#define nsSystemPrincipal_h__

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"

#include "mozilla/BasePrincipal.h"

#define NS_SYSTEMPRINCIPAL_CID \
{ 0x4a6212db, 0xaccb, 0x11d3, \
{ 0xb7, 0x65, 0x0, 0x60, 0xb0, 0xb6, 0xce, 0xcb }}
#define NS_SYSTEMPRINCIPAL_CONTRACTID "@mozilla.org/systemprincipal;1"


class nsSystemPrincipal final : public mozilla::BasePrincipal
{
public:
  NS_DECL_NSISERIALIZABLE
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;
  NS_IMETHOD Equals(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD EqualsConsideringDomain(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD GetHashValue(uint32_t* aHashValue) override;
  NS_IMETHOD GetURI(nsIURI** aURI) override;
  NS_IMETHOD GetDomain(nsIURI** aDomain) override;
  NS_IMETHOD SetDomain(nsIURI* aDomain) override;
  NS_IMETHOD GetOrigin(nsACString& aOrigin) override;
  NS_IMETHOD Subsumes(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD SubsumesConsideringDomain(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD CheckMayLoad(nsIURI* uri, bool report, bool allowIfInheritsPrincipal) override;
  NS_IMETHOD GetCsp(nsIContentSecurityPolicy** aCsp) override;
  NS_IMETHOD SetCsp(nsIContentSecurityPolicy* aCsp) override;
  NS_IMETHOD GetBaseDomain(nsACString& aBaseDomain) override;

  nsSystemPrincipal() {}

  virtual void GetScriptLocation(nsACString &aStr) override;

protected:
  virtual ~nsSystemPrincipal(void) {}
};

#endif 
