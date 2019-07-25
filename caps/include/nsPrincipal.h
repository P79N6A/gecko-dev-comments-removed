






































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

class nsPrincipal : public nsJSPrincipals
{
public:
  nsPrincipal();

protected:
  virtual ~nsPrincipal();

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
public:

  NS_DECL_NSIPRINCIPAL
  NS_DECL_NSISERIALIZABLE

  
  
  nsresult Init(const nsACString& aCertFingerprint,
                const nsACString& aSubjectName,
                const nsACString& aPrettyName,
                nsISupports* aCert,
                nsIURI *aCodebase);
  nsresult InitFromPersistent(const char* aPrefName,
                              const nsCString& aFingerprint,
                              const nsCString& aSubjectName,
                              const nsACString& aPrettyName,
                              const char* aGrantedList,
                              const char* aDeniedList,
                              nsISupports* aCert,
                              bool aIsCert,
                              bool aTrusted);

  
  
  
  nsresult EnsureCertData(const nsACString& aSubjectName,
                          const nsACString& aPrettyName,
                          nsISupports* aCert);

  enum AnnotationValue { AnnotationEnabled=1, AnnotationDisabled };

  void SetURI(nsIURI *aURI);
  nsresult SetCapability(const char *capability, void **annotation, 
                         AnnotationValue value);

  static const char sInvalid[];

  virtual void GetScriptLocation(nsACString &aStr) MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void dumpImpl() MOZ_OVERRIDE;
#endif 

protected:
  
  nsresult SetCanEnableCapability(const char *capability, PRInt16 canEnable);

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

  
  
  
  nsAutoPtr<Certificate> mCert;

  DomainPolicy* mSecurityPolicy;

  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
  nsCOMPtr<nsIURI> mCodebase;
  nsCOMPtr<nsIURI> mDomain;
  bool mTrusted;
  bool mInitialized;
  
  bool mCodebaseImmutable;
  bool mDomainImmutable;
};


#define NS_PRINCIPAL_CLASSNAME  "principal"
#define NS_PRINCIPAL_CONTRACTID "@mozilla.org/principal;1"
#define NS_PRINCIPAL_CID \
  { 0x36102b6b, 0x7b62, 0x451a, \
    { 0xa1, 0xc8, 0xa0, 0xd4, 0x56, 0xc9, 0x2d, 0xc5 }}


#endif 
