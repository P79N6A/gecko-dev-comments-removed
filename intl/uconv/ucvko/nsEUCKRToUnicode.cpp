




































#include "nsEUCKRToUnicode.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"

NS_METHOD
nsEUCKRToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return nsCP949ToUnicodeConstructor(aOuter, aIID, aResult);
}
