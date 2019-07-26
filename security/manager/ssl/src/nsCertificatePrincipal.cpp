




#include "nsCertificatePrincipal.h"

NS_IMPL_ISUPPORTS1(nsCertificatePrincipal, nsICertificatePrincipal)

NS_IMETHODIMP
nsCertificatePrincipal::GetFingerprint(nsACString& aFingerprint)
{
  aFingerprint = mFingerprint;
  return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::GetSubjectName(nsACString& aSubjectName)
{
  aSubjectName = mSubjectName;
  return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::GetPrettyName(nsACString& aPrettyName)
{
  aPrettyName = mPrettyName;
  return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::GetCertificate(nsISupports** aCert)
{
  nsCOMPtr<nsISupports> cert = mCert;
  cert.forget(aCert);
  return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::GetHasCertificate(bool* rv)
{
  *rv = true;
  return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::Equals(nsICertificatePrincipal* aOther, bool* rv)
{
  nsAutoCString str;
  aOther->GetFingerprint(str);
  if (!str.Equals(mFingerprint)) {
    *rv = false;
    return NS_OK;
  }

  
  
  if (!mSubjectName.IsEmpty()) {
    
    aOther->GetSubjectName(str);
    *rv = str.Equals(mSubjectName) || str.IsEmpty();
    return NS_OK;
  }

  *rv = true;
  return NS_OK;
}
