




#ifndef nsPrincipal_h__
#define nsPrincipal_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsJSPrincipals.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIProtocolHandler.h"
#include "nsNetUtil.h"
#include "nsScriptSecurityManager.h"
#include "mozilla/BasePrincipal.h"

class nsPrincipal final : public mozilla::BasePrincipal
{
public:
  NS_DECL_NSISERIALIZABLE
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;
  NS_IMETHOD GetHashValue(uint32_t* aHashValue) override;
  NS_IMETHOD GetURI(nsIURI** aURI) override;
  NS_IMETHOD GetDomain(nsIURI** aDomain) override;
  NS_IMETHOD SetDomain(nsIURI* aDomain) override;
  NS_IMETHOD CheckMayLoad(nsIURI* uri, bool report, bool allowIfInheritsPrincipal) override;
  NS_IMETHOD GetBaseDomain(nsACString& aBaseDomain) override;
  virtual bool IsOnCSSUnprefixingWhitelist() override;
  bool IsCodebasePrincipal() const override { return true; }
  nsresult GetOriginInternal(nsACString& aOrigin) override;

  nsPrincipal();

  
  nsresult Init(nsIURI* aCodebase, const mozilla::OriginAttributes& aOriginAttributes);

  virtual void GetScriptLocation(nsACString& aStr) override;
  void SetURI(nsIURI* aURI);

  static bool IsPrincipalInherited(nsIURI* aURI) {
    
    
    bool doesInheritSecurityContext;
    nsresult rv =
    NS_URIChainHasFlags(aURI,
                        nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                        &doesInheritSecurityContext);

    if (NS_SUCCEEDED(rv) && doesInheritSecurityContext) {
      return true;
    }

    return false;
  }


  


  static nsresult GetOriginForURI(nsIURI* aURI, nsACString& aOrigin);

  


  static void InitializeStatics();

  nsCOMPtr<nsIURI> mDomain;
  nsCOMPtr<nsIURI> mCodebase;
  
  bool mCodebaseImmutable;
  bool mDomainImmutable;
  bool mInitialized;
  mozilla::Maybe<bool> mIsOnCSSUnprefixingWhitelist; 

protected:
  virtual ~nsPrincipal();

  bool SubsumesInternal(nsIPrincipal* aOther, DocumentDomainConsideration aConsideration) override;
};

class nsExpandedPrincipal : public nsIExpandedPrincipal, public mozilla::BasePrincipal
{
public:
  explicit nsExpandedPrincipal(nsTArray< nsCOMPtr<nsIPrincipal> > &aWhiteList);

  NS_DECL_NSIEXPANDEDPRINCIPAL
  NS_DECL_NSISERIALIZABLE
  NS_IMETHODIMP_(MozExternalRefCountType) AddRef() override { return nsJSPrincipals::AddRef(); };
  NS_IMETHODIMP_(MozExternalRefCountType) Release() override { return nsJSPrincipals::Release(); };
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;
  NS_IMETHOD GetHashValue(uint32_t* aHashValue) override;
  NS_IMETHOD GetURI(nsIURI** aURI) override;
  NS_IMETHOD GetDomain(nsIURI** aDomain) override;
  NS_IMETHOD SetDomain(nsIURI* aDomain) override;
  NS_IMETHOD CheckMayLoad(nsIURI* uri, bool report, bool allowIfInheritsPrincipal) override;
  NS_IMETHOD GetBaseDomain(nsACString& aBaseDomain) override;
  virtual bool IsOnCSSUnprefixingWhitelist() override;
  virtual void GetScriptLocation(nsACString &aStr) override;
  nsresult GetOriginInternal(nsACString& aOrigin) override;

protected:
  virtual ~nsExpandedPrincipal();

  bool SubsumesInternal(nsIPrincipal* aOther, DocumentDomainConsideration aConsideration) override;

private:
  nsTArray< nsCOMPtr<nsIPrincipal> > mPrincipals;
};

#define NS_PRINCIPAL_CONTRACTID "@mozilla.org/principal;1"
#define NS_PRINCIPAL_CID \
  { 0xb7c8505e, 0xc56d, 0x4191, \
    { 0xa1, 0x5e, 0x5d, 0xcb, 0x88, 0x9b, 0xa0, 0x94 }}

#define NS_EXPANDEDPRINCIPAL_CONTRACTID "@mozilla.org/expandedprincipal;1"
#define NS_EXPANDEDPRINCIPAL_CID \
  { 0x38539471, 0x68cc, 0x4a6f, \
    { 0x81, 0x20, 0xdb, 0xd5, 0x4a, 0x22, 0x0a, 0x13 }}

#endif 
