




































#include "nsUCConstructors.h"
#include "nsISO885914ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-14.ut"
};

NS_METHOD
nsISO885914ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
