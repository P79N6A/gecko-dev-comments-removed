




#ifndef nsPrincipal_h__
#define nsPrincipal_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsJSPrincipals.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsIProtocolHandler.h"
#include "nsNetUtil.h"
#include "nsScriptSecurityManager.h"

class nsBasePrincipal : public nsJSPrincipals
{
public:
  nsBasePrincipal();

protected:
  virtual ~nsBasePrincipal();

public:
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void);
  NS_IMETHOD_(MozExternalRefCountType) Release(void);
  NS_IMETHOD GetCsp(nsIContentSecurityPolicy** aCsp);
  NS_IMETHOD SetCsp(nsIContentSecurityPolicy* aCsp);
public:

  static const char sInvalid[];

protected:

#ifdef DEBUG
  virtual void dumpImpl() = 0;
#endif

  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
};

class nsPrincipal final : public nsBasePrincipal
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISERIALIZABLE
  NS_IMETHOD Equals(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD EqualsConsideringDomain(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD GetHashValue(uint32_t* aHashValue) override;
  NS_IMETHOD GetURI(nsIURI** aURI) override;
  NS_IMETHOD GetDomain(nsIURI** aDomain) override;
  NS_IMETHOD SetDomain(nsIURI* aDomain) override;
  NS_IMETHOD GetOrigin(char** aOrigin) override;
  NS_IMETHOD Subsumes(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD SubsumesConsideringDomain(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD CheckMayLoad(nsIURI* uri, bool report, bool allowIfInheritsPrincipal) override;
  NS_IMETHOD GetJarPrefix(nsACString& aJarPrefix) override;
  NS_IMETHOD GetAppStatus(uint16_t* aAppStatus) override;
  NS_IMETHOD GetAppId(uint32_t* aAppStatus) override;
  NS_IMETHOD GetIsInBrowserElement(bool* aIsInBrowserElement) override;
  NS_IMETHOD GetUnknownAppId(bool* aUnknownAppId) override;
  NS_IMETHOD GetIsNullPrincipal(bool* aIsNullPrincipal) override;
  NS_IMETHOD GetBaseDomain(nsACString& aBaseDomain) override;
  virtual bool IsOnCSSUnprefixingWhitelist() override;
#ifdef DEBUG
  virtual void dumpImpl() override;
#endif

  nsPrincipal();

  
  nsresult Init(nsIURI* aCodebase,
                uint32_t aAppId,
                bool aInMozBrowser);

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


  


  static nsresult GetOriginForURI(nsIURI* aURI, char **aOrigin);

  


  static void InitializeStatics();

  nsCOMPtr<nsIURI> mDomain;
  nsCOMPtr<nsIURI> mCodebase;
  uint32_t mAppId;
  bool mInMozBrowser;
  
  bool mCodebaseImmutable;
  bool mDomainImmutable;
  bool mInitialized;
  mozilla::Maybe<bool> mIsOnCSSUnprefixingWhitelist; 

protected:
  virtual ~nsPrincipal();

  


  uint16_t GetAppStatus();
};

class nsExpandedPrincipal : public nsIExpandedPrincipal, public nsBasePrincipal
{
public:
  explicit nsExpandedPrincipal(nsTArray< nsCOMPtr<nsIPrincipal> > &aWhiteList);

protected:
  virtual ~nsExpandedPrincipal();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIEXPANDEDPRINCIPAL
  NS_DECL_NSISERIALIZABLE
  NS_IMETHOD Equals(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD EqualsConsideringDomain(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD GetHashValue(uint32_t* aHashValue) override;
  NS_IMETHOD GetURI(nsIURI** aURI) override;
  NS_IMETHOD GetDomain(nsIURI** aDomain) override;
  NS_IMETHOD SetDomain(nsIURI* aDomain) override;
  NS_IMETHOD GetOrigin(char** aOrigin) override;
  NS_IMETHOD Subsumes(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD SubsumesConsideringDomain(nsIPrincipal* other, bool* _retval) override;
  NS_IMETHOD CheckMayLoad(nsIURI* uri, bool report, bool allowIfInheritsPrincipal) override;
  NS_IMETHOD GetJarPrefix(nsACString& aJarPrefix) override;
  NS_IMETHOD GetAppStatus(uint16_t* aAppStatus) override;
  NS_IMETHOD GetAppId(uint32_t* aAppStatus) override;
  NS_IMETHOD GetIsInBrowserElement(bool* aIsInBrowserElement) override;
  NS_IMETHOD GetUnknownAppId(bool* aUnknownAppId) override;
  NS_IMETHOD GetIsNullPrincipal(bool* aIsNullPrincipal) override;
  NS_IMETHOD GetBaseDomain(nsACString& aBaseDomain) override;
  virtual bool IsOnCSSUnprefixingWhitelist() override;
#ifdef DEBUG
  virtual void dumpImpl() override;
#endif
  
  virtual void GetScriptLocation(nsACString &aStr) override;

private:
  nsTArray< nsCOMPtr<nsIPrincipal> > mPrincipals;
};

#define NS_PRINCIPAL_CONTRACTID "@mozilla.org/principal;1"
#define NS_PRINCIPAL_CID \
  { 0x09b7e598, 0x490d, 0x423f, \
    { 0xa8, 0xa6, 0x2e, 0x6c, 0x4e, 0xc8, 0x77, 0x50 }}

#define NS_EXPANDEDPRINCIPAL_CONTRACTID "@mozilla.org/expandedprincipal;1"
#define NS_EXPANDEDPRINCIPAL_CID \
  { 0xb33a3807, 0xb76c, 0x44e5, \
    { 0xb9, 0x9d, 0x95, 0x7e, 0xe9, 0xba, 0x6e, 0x39 }}

#endif 
