




#include "nsScriptableBase64Encoder.h"
#include "mozilla/Base64.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS(nsScriptableBase64Encoder, nsIScriptableBase64Encoder)


NS_IMETHODIMP
nsScriptableBase64Encoder::EncodeToCString(nsIInputStream* aStream,
                                           uint32_t aLength,
                                           nsACString& aResult)
{
  Base64EncodeInputStream(aStream, aResult, aLength);
  return NS_OK;
}


NS_IMETHODIMP
nsScriptableBase64Encoder::EncodeToString(nsIInputStream* aStream,
                                          uint32_t aLength,
                                          nsAString& aResult)
{
  Base64EncodeInputStream(aStream, aResult, aLength);
  return NS_OK;
}
