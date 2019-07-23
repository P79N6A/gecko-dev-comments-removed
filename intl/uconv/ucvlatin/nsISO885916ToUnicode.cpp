




































#include "nsUCConstructors.h"
#include "nsISO885916ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-16.ut"
};

NS_METHOD
nsISO885916ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
