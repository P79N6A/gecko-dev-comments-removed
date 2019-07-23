






































#ifndef nsPrincipal_h__
#define nsPrincipal_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsHashtable.h"
#include "nsJSPrincipals.h"

class nsIObjectInputStream;
class nsIObjectOutputStream;

class nsPrincipal : public nsIPrincipal
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
                              PRBool aIsCert,
                              PRBool aTrusted);

  
  
  
  nsresult EnsureCertData(const nsACString& aSubjectName,
                          const nsACString& aPrettyName,
                          nsISupports* aCert);

  enum AnnotationValue { AnnotationEnabled=1, AnnotationDisabled };

  void SetURI(nsIURI *aURI);
  nsresult SetCapability(const char *capability, void **annotation, 
                         AnnotationValue value);

  static const char sInvalid[];

protected:
  nsJSPrincipals mJSPrincipals;
  nsVoidArray mAnnotations;
  nsHashtable mCapabilities;
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

  
  
  
  nsAutoPtr<Certificate> mCert;

  DomainPolicy* mSecurityPolicy;

  nsCOMPtr<nsIURI> mCodebase;
  nsCOMPtr<nsIURI> mDomain;
  nsCOMPtr<nsIURI> mOrigin;
  PRPackedBool mTrusted;
  PRPackedBool mInitialized;
};


#define NS_PRINCIPAL_CLASSNAME  "principal"
#define NS_PRINCIPAL_CONTRACTID "@mozilla.org/principal;1"
#define NS_PRINCIPAL_CID \
  { 0x36102b6b, 0x7b62, 0x451a, \
    { 0xa1, 0xc8, 0xa0, 0xd4, 0x56, 0xc9, 0x2d, 0xc5 }}


#endif 
