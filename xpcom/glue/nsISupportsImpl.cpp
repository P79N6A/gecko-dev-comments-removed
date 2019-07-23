




































#include "nsISupportsImpl.h"

nsresult NS_FASTCALL
NS_TableDrivenQI(void* aThis, const QITableEntry* entries,
                 REFNSIID aIID, void **aInstancePtr)
{
  while (entries->iid) {
    if (aIID.Equals(*entries->iid)) {
      nsISupports* r =
        NS_REINTERPRET_CAST(nsISupports*,
          NS_REINTERPRET_CAST(char*, aThis) + entries->offset);
      NS_ADDREF(r);
      *aInstancePtr = r;
      return NS_OK;
    }

    ++entries;
  }

  *aInstancePtr = nsnull;
  return NS_ERROR_NO_INTERFACE;
}
