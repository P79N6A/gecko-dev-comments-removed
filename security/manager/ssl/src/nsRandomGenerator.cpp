



#include "nsRandomGenerator.h"
#include "pk11pub.h"
#include "secerr.h"
#include "prerror.h"
#include "nsNSSComponent.h"




NS_IMPL_THREADSAFE_ISUPPORTS1(nsRandomGenerator, nsIRandomGenerator)






NS_IMETHODIMP
nsRandomGenerator::GenerateRandomBytes(uint32_t aLength,
                                       uint8_t **aBuffer)
{
  NS_ENSURE_ARG_POINTER(aBuffer);
  *aBuffer = nullptr;

  uint8_t *buf = reinterpret_cast<uint8_t *>(NS_Alloc(aLength));
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY;

  mozilla::ScopedPK11SlotInfo slot(PK11_GetInternalSlot());
  if (slot == NULL) {
    return NS_ERROR_FAILURE;
  }
  SECStatus srv = PK11_GenerateRandomOnSlot(slot, buf, aLength);

  if (SECSuccess != srv) {
    NS_Free(buf);
    return NS_ERROR_FAILURE;
  }

  *aBuffer = buf;

  return NS_OK;
}
