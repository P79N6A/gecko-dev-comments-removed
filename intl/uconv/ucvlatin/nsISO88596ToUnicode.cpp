




































#include "nsUCConstructors.h"
#include "nsISO88596ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-6.ut"
};

NS_METHOD
nsISO88596ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

