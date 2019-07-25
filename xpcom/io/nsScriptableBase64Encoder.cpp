




































#include "nsScriptableBase64Encoder.h"
#include "mozilla/Base64.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsScriptableBase64Encoder, nsIScriptableBase64Encoder)


NS_IMETHODIMP
nsScriptableBase64Encoder::EncodeToCString(nsIInputStream *aStream,
                                           PRUint32 aLength,
                                           nsACString & _retval)
{
  Base64EncodeInputStream(aStream, _retval, aLength);
  return NS_OK;
}


NS_IMETHODIMP
nsScriptableBase64Encoder::EncodeToString(nsIInputStream *aStream,
                                          PRUint32 aLength,
                                          nsAString & _retval)
{
  Base64EncodeInputStream(aStream, _retval, aLength);
  return NS_OK;
}
