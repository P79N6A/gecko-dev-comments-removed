




































#include "nsUCConstructors.h"
#include "nsISO88599ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-9.ut"
};

NS_METHOD
nsISO88599ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
