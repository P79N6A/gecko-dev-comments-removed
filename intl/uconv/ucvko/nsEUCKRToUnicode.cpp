




































#include "nsEUCKRToUnicode.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"

nsresult
nsEUCKRToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return nsCP949ToUnicodeConstructor(aOuter, aIID, aResult);
}
