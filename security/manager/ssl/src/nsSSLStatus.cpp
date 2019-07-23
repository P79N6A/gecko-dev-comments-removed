






































#include "nsSSLStatus.h"
#include "plstr.h"

NS_IMETHODIMP
nsSSLStatus::GetServerCert(nsIX509Cert** _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = mServerCert;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetKeyLength(PRUint32* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");
  if (!mHaveKeyLengthAndCipher)
    return NS_ERROR_NOT_AVAILABLE;

  *_result = mKeyLength;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetSecretKeyLength(PRUint32* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");
  if (!mHaveKeyLengthAndCipher)
    return NS_ERROR_NOT_AVAILABLE;

  *_result = mSecretKeyLength;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetCipherName(char** _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");
  if (!mHaveKeyLengthAndCipher)
    return NS_ERROR_NOT_AVAILABLE;

  *_result = PL_strdup(mCipherName.get());

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetIsDomainMismatch(PRBool* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");
  if (!mHaveCertStatus)
    return NS_ERROR_NOT_AVAILABLE;

  *_result = mIsDomainMismatch;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetIsNotValidAtThisTime(PRBool* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");
  if (!mHaveCertStatus)
    return NS_ERROR_NOT_AVAILABLE;

  *_result = mIsNotValidAtThisTime;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetIsUntrusted(PRBool* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");
  if (!mHaveCertStatus)
    return NS_ERROR_NOT_AVAILABLE;

  *_result = mIsUntrusted;

  return NS_OK;
}

nsSSLStatus::nsSSLStatus()
: mKeyLength(0), mSecretKeyLength(0)
, mIsDomainMismatch(PR_FALSE)
, mIsNotValidAtThisTime(PR_FALSE)
, mIsUntrusted(PR_FALSE)
, mHaveKeyLengthAndCipher(PR_FALSE)
, mHaveCertStatus(PR_FALSE)
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSSLStatus, nsISSLStatus)

nsSSLStatus::~nsSSLStatus()
{
}
