






































#include "nsSSLStatus.h"
#include "plstr.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"
#include "nsIObjectOutputStream.h"
#include "nsIObjectInputStream.h"

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

  *_result = ToNewCString(mCipherName);

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetIsDomainMismatch(PRBool* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = mHaveCertErrorBits && mIsDomainMismatch;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetIsNotValidAtThisTime(PRBool* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = mHaveCertErrorBits && mIsNotValidAtThisTime;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetIsUntrusted(PRBool* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = mHaveCertErrorBits && mIsUntrusted;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::Read(nsIObjectInputStream* stream)
{
  nsCOMPtr<nsISupports> cert;
  nsresult rv = stream->ReadObject(PR_TRUE, getter_AddRefs(cert));
  NS_ENSURE_SUCCESS(rv, rv);

  mServerCert = do_QueryInterface(cert);
  if (!mServerCert)
    return NS_NOINTERFACE;

  rv = stream->Read32(&mKeyLength);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->Read32(&mSecretKeyLength);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->ReadCString(mCipherName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->ReadBoolean(&mIsDomainMismatch);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->ReadBoolean(&mIsNotValidAtThisTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->ReadBoolean(&mIsUntrusted);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->ReadBoolean(&mHaveKeyLengthAndCipher);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->ReadBoolean(&mHaveCertErrorBits);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::Write(nsIObjectOutputStream* stream)
{
  nsresult rv = stream->WriteCompoundObject(mServerCert,
                                            NS_GET_IID(nsIX509Cert),
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->Write32(mKeyLength);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->Write32(mSecretKeyLength);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->WriteStringZ(mCipherName.get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->WriteBoolean(mIsDomainMismatch);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->WriteBoolean(mIsNotValidAtThisTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->WriteBoolean(mIsUntrusted);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->WriteBoolean(mHaveKeyLengthAndCipher);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stream->WriteBoolean(mHaveCertErrorBits);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetInterfaces(PRUint32 *count, nsIID * **array)
{
  *count = 0;
  *array = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetContractID(char * *aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetClassID(nsCID * *aClassID)
{
  *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
  if (!*aClassID)
    return NS_ERROR_OUT_OF_MEMORY;
  return GetClassIDNoAlloc(*aClassID);
}

NS_IMETHODIMP
nsSSLStatus::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

static NS_DEFINE_CID(kSSLStatusCID, NS_SSLSTATUS_CID);

NS_IMETHODIMP
nsSSLStatus::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  *aClassIDNoAlloc = kSSLStatusCID;
  return NS_OK;
}



nsSSLStatus::nsSSLStatus()
: mKeyLength(0), mSecretKeyLength(0)
, mIsDomainMismatch(PR_FALSE)
, mIsNotValidAtThisTime(PR_FALSE)
, mIsUntrusted(PR_FALSE)
, mHaveKeyLengthAndCipher(PR_FALSE)
, mHaveCertErrorBits(PR_FALSE)
{
  mCipherName = "";
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsSSLStatus, nsISSLStatus, nsISerializable, nsIClassInfo)

nsSSLStatus::~nsSSLStatus()
{
}
