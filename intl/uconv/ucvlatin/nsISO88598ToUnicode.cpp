




































#include "nsUCConstructors.h"
#include "nsISO88598ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-8.ut"
};

NS_METHOD
nsISO88598ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

