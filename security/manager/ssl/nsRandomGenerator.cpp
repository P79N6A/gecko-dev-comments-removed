



#include "nsRandomGenerator.h"

#include "nsNSSComponent.h"
#include "pk11pub.h"
#include "prerror.h"
#include "secerr.h"

NS_IMPL_ISUPPORTS(nsRandomGenerator, nsIRandomGenerator)



NS_IMETHODIMP
nsRandomGenerator::GenerateRandomBytes(uint32_t aLength,
                                       uint8_t** aBuffer)
{
  NS_ENSURE_ARG_POINTER(aBuffer);
  *aBuffer = nullptr;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  mozilla::ScopedPK11SlotInfo slot(PK11_GetInternalSlot());
  if (!slot) {
    return NS_ERROR_FAILURE;
  }

  uint8_t* buf = reinterpret_cast<uint8_t*>(moz_xmalloc(aLength));
  if (!buf) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  SECStatus srv = PK11_GenerateRandomOnSlot(slot, buf, aLength);

  if (srv != SECSuccess) {
    free(buf);
    return NS_ERROR_FAILURE;
  }

  *aBuffer = buf;

  return NS_OK;
}

nsRandomGenerator::~nsRandomGenerator()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return;
  }
  shutdown(calledFromObject);
}
