




#include "nsISO88599ToUnicode.h"
#include "nsCP1254ToUnicode.h"

nsresult
nsISO88599ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  
  return nsCP1254ToUnicodeConstructor(aOuter, aIID, aResult);
}
