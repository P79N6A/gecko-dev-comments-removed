





#ifndef mozilla_BasePrincipal_h
#define mozilla_BasePrincipal_h

#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsJSPrincipals.h"

#include "mozilla/dom/SystemDictionariesBinding.h"

class nsIObjectOutputStream;
class nsIObjectInputStream;

namespace mozilla {

class OriginAttributes : public dom::OriginAttributesDictionary
{
public:
  OriginAttributes() {}
  OriginAttributes(uint32_t aAppId, bool aInBrowser)
  {
    mAppId = aAppId;
    mInBrowser = aInBrowser;
  }

  bool operator==(const OriginAttributes& aOther) const
  {
    return mAppId == aOther.mAppId &&
           mInBrowser == aOther.mInBrowser;
  }
  bool operator!=(const OriginAttributes& aOther) const
  {
    return !(*this == aOther);
  }

  
  
  
  void CreateSuffix(nsACString& aStr);

  void Serialize(nsIObjectOutputStream* aStream) const;
  nsresult Deserialize(nsIObjectInputStream* aStream);
};








class BasePrincipal : public nsJSPrincipals
{
public:
  BasePrincipal() {}

  enum DocumentDomainConsideration { DontConsiderDocumentDomain, ConsiderDocumentDomain};
  bool Subsumes(nsIPrincipal* aOther, DocumentDomainConsideration aConsideration);

  NS_IMETHOD Equals(nsIPrincipal* other, bool* _retval) final;
  NS_IMETHOD EqualsConsideringDomain(nsIPrincipal* other, bool* _retval) final;
  NS_IMETHOD Subsumes(nsIPrincipal* other, bool* _retval) final;
  NS_IMETHOD SubsumesConsideringDomain(nsIPrincipal* other, bool* _retval) final;
  NS_IMETHOD GetCsp(nsIContentSecurityPolicy** aCsp) override;
  NS_IMETHOD SetCsp(nsIContentSecurityPolicy* aCsp) override;
  NS_IMETHOD GetIsNullPrincipal(bool* aIsNullPrincipal) override;
  NS_IMETHOD GetJarPrefix(nsACString& aJarPrefix) final;
  NS_IMETHOD GetOriginAttributes(JSContext* aCx, JS::MutableHandle<JS::Value> aVal) final;
  NS_IMETHOD GetOriginSuffix(nsACString& aOriginSuffix) final;
  NS_IMETHOD GetAppStatus(uint16_t* aAppStatus) final;
  NS_IMETHOD GetAppId(uint32_t* aAppStatus) final;
  NS_IMETHOD GetIsInBrowserElement(bool* aIsInBrowserElement) final;
  NS_IMETHOD GetUnknownAppId(bool* aUnknownAppId) final;

  virtual bool IsOnCSSUnprefixingWhitelist() override { return false; }

  static BasePrincipal* Cast(nsIPrincipal* aPrin) { return static_cast<BasePrincipal*>(aPrin); }
  static already_AddRefed<BasePrincipal> CreateCodebasePrincipal(nsIURI* aURI, OriginAttributes& aAttrs);

  const OriginAttributes& OriginAttributesRef() { return mOriginAttributes; }
  uint32_t AppId() const { return mOriginAttributes.mAppId; }
  bool IsInBrowserElement() const { return mOriginAttributes.mInBrowser; }

protected:
  virtual ~BasePrincipal() {}

  virtual bool SubsumesInternal(nsIPrincipal* aOther, DocumentDomainConsideration aConsider) = 0;

  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
  OriginAttributes mOriginAttributes;
};

} 

#endif 
