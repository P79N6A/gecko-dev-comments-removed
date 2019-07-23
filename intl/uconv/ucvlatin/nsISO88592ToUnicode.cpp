




































#include "nsUCConstructors.h"
#include "nsISO88592ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-2.ut"
};

NS_METHOD
nsISO88592ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

