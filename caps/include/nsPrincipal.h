




#ifndef nsPrincipal_h__
#define nsPrincipal_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsHashtable.h"
#include "nsJSPrincipals.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class nsIObjectInputStream;
class nsIObjectOutputStream;

class nsBasePrincipal : public nsJSPrincipals
{
public:
  nsBasePrincipal();

protected:
  virtual ~nsBasePrincipal();

public:
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);
  NS_SCRIPTABLE NS_IMETHOD GetPreferences(char** prefBranch NS_OUTPARAM, char** id NS_OUTPARAM, char** subjectName NS_OUTPARAM, char** grantedList NS_OUTPARAM, char** deniedList NS_OUTPARAM, bool* isTrusted NS_OUTPARAM);
  NS_IMETHOD GetSecurityPolicy(void** aSecurityPolicy);
  NS_IMETHOD SetSecurityPolicy(void* aSecurityPolicy);
  NS_IMETHOD CanEnableCapability(const char* capability, PRInt16* _retval NS_OUTPARAM);
  NS_IMETHOD IsCapabilityEnabled(const char* capability, void* annotation, bool* _retval NS_OUTPARAM);
  NS_IMETHOD EnableCapability(const char* capability, void** annotation NS_INOUTPARAM);
  NS_IMETHOD GetHasCertificate(bool* aHasCertificate);
  NS_IMETHOD GetFingerprint(nsACString& aFingerprint);
  NS_IMETHOD GetPrettyName(nsACString& aPrettyName);
  NS_IMETHOD GetSubjectName(nsACString& aSubjectName);
  NS_IMETHOD GetCertificate(nsISupports** aCertificate);
  NS_IMETHOD GetCsp(nsIContentSecurityPolicy** aCsp);
  NS_IMETHOD SetCsp(nsIContentSecurityPolicy* aCsp);
public:

  
  
  
  nsresult EnsureCertData(const nsACString& aSubjectName,
                          const nsACString& aPrettyName,
                          nsISupports* aCert);

  enum AnnotationValue { AnnotationEnabled=1, AnnotationDisabled };

  nsresult SetCapability(const char* capability, void** annotation, 
                         AnnotationValue value);

  static const char sInvalid[];

protected:
  
  nsresult SetCanEnableCapability(const char* capability, PRInt16 canEnable);

  nsTArray< nsAutoPtr<nsHashtable> > mAnnotations;
  nsHashtable* mCapabilities;
  nsCString mPrefName;
  static PRInt32 sCapabilitiesOrdinal;

  
  
  
  struct Certificate
  {
    Certificate(const nsACString& aFingerprint, const nsACString& aSubjectName,
                const nsACString& aPrettyName, nsISupports* aCert)
      : fingerprint(aFingerprint),
        subjectName(aSubjectName),
        prettyName(aPrettyName),
        cert(aCert)
    {
    }
    nsCString fingerprint;
    nsCString subjectName;
    nsCString prettyName;
    nsCOMPtr<nsISupports> cert;
  };

  nsresult SetCertificate(const nsACString& aFingerprint,
                          const nsACString& aSubjectName,
                          const nsACString& aPrettyName,
                          nsISupports* aCert);

  
  bool CertificateEquals(nsIPrincipal *aOther);

#ifdef DEBUG
  virtual void dumpImpl() = 0;
#endif

  
  
  
  nsAutoPtr<Certificate> mCert;

  DomainPolicy* mSecurityPolicy;

  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
  bool mTrusted;
};

class nsPrincipal : public nsBasePrincipal
{
public:
  nsPrincipal();

protected:
  virtual ~nsPrincipal();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISERIALIZABLE
  NS_IMETHOD Equals(nsIPrincipal* other, bool* _retval NS_OUTPARAM);
  NS_IMETHOD EqualsIgnoringDomain(nsIPrincipal* other, bool* _retval NS_OUTPARAM);
  NS_IMETHOD GetHashValue(PRUint32* aHashValue);
  NS_IMETHOD GetURI(nsIURI** aURI);
  NS_IMETHOD GetDomain(nsIURI** aDomain);
  NS_IMETHOD SetDomain(nsIURI* aDomain);
  NS_IMETHOD GetOrigin(char** aOrigin);
  NS_IMETHOD Subsumes(nsIPrincipal* other, bool* _retval NS_OUTPARAM);
  NS_IMETHOD SubsumesIgnoringDomain(nsIPrincipal* other, bool* _retval NS_OUTPARAM);
  NS_IMETHOD CheckMayLoad(nsIURI* uri, bool report);
#ifdef DEBUG
  virtual void dumpImpl();
#endif
  
  
  nsresult Init(const nsACString& aCertFingerprint,
                const nsACString& aSubjectName,
                const nsACString& aPrettyName,
                nsISupports* aCert,
                nsIURI* aCodebase);
  nsresult InitFromPersistent(const char* aPrefName,
                              const nsCString& aFingerprint,
                              const nsCString& aSubjectName,
                              const nsACString& aPrettyName,
                              const char* aGrantedList,
                              const char* aDeniedList,
                              nsISupports* aCert,
                              bool aIsCert,
                              bool aTrusted);

  virtual void GetScriptLocation(nsACString& aStr) MOZ_OVERRIDE;
  void SetURI(nsIURI* aURI);

  nsCOMPtr<nsIURI> mDomain;
  nsCOMPtr<nsIURI> mCodebase;
  
  bool mCodebaseImmutable;
  bool mDomainImmutable;
  bool mInitialized;
};


#define NS_PRINCIPAL_CLASSNAME  "principal"
#define NS_PRINCIPAL_CONTRACTID "@mozilla.org/principal;1"
#define NS_PRINCIPAL_CID \
  { 0x36102b6b, 0x7b62, 0x451a, \
    { 0xa1, 0xc8, 0xa0, 0xd4, 0x56, 0xc9, 0x2d, 0xc5 }}


#endif 
