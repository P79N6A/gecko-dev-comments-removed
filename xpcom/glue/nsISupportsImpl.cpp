





#include "nsISupportsImpl.h"

nsresult NS_FASTCALL
NS_TableDrivenQI(void* aThis, REFNSIID aIID, void** aInstancePtr,
                 const QITableEntry* aEntries)
{
  do {
    if (aIID.Equals(*aEntries->iid)) {
      nsISupports* r = reinterpret_cast<nsISupports*>(
        reinterpret_cast<char*>(aThis) + aEntries->offset);
      NS_ADDREF(r);
      *aInstancePtr = r;
      return NS_OK;
    }

    ++aEntries;
  } while (aEntries->iid);

  *aInstancePtr = nullptr;
  return NS_ERROR_NO_INTERFACE;
}
