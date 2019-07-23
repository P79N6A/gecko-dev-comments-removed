




































#include "nsUCConstructors.h"
#include "nsISO88593ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-3.ut"
};

NS_METHOD
nsISO88593ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

