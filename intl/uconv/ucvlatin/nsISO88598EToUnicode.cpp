




#include "nsUCConstructors.h"
#include "nsISO88598ToUnicode.h"
#include "nsISO88598EToUnicode.h"




nsresult
nsISO88598EToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return nsISO88598ToUnicodeConstructor(aOuter, aIID, aResult);
}

