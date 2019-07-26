




#ifndef __NS_CERTIFICATEPRINCIPAL_H
#define __NS_CERTIFICATEPRINCIPAL_H

#include "nsICertificatePrincipal.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsCertificatePrincipal : public nsICertificatePrincipal
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICERTIFICATEPRINCIPAL

  nsCertificatePrincipal(const nsACString& aFingerprint,
                         const nsACString& aSubjectName,
                         const nsACString& aPrettyName,
                         nsISupports* aCert)
                         : mFingerprint(aFingerprint)
                         , mSubjectName(aSubjectName)
                         , mPrettyName(aPrettyName)
                         , mCert(aCert)
    {}

  virtual ~nsCertificatePrincipal() {};

private:
  nsCString             mFingerprint;
  nsCString             mSubjectName;
  nsCString             mPrettyName;
  nsCOMPtr<nsISupports> mCert;
};

#endif 
