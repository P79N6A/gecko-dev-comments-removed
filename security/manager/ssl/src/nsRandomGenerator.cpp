




































#include "nsRandomGenerator.h"
#include "pk11pub.h"




NS_IMPL_THREADSAFE_ISUPPORTS1(nsRandomGenerator, nsIRandomGenerator)






NS_IMETHODIMP
nsRandomGenerator::GenerateRandomBytes(PRUint32 aLength,
                                       PRUint8 **aBuffer)
{
  NS_ENSURE_ARG_POINTER(aBuffer);

  PRUint8 *buf = reinterpret_cast<PRUint8 *>(NS_Alloc(aLength));
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY;

  SECStatus srv = PK11_GenerateRandom(buf, aLength);
  if (SECSuccess != srv) {
    NS_Free(buf);
    return NS_ERROR_FAILURE;
  }

  *aBuffer = buf;

  return NS_OK;
}
