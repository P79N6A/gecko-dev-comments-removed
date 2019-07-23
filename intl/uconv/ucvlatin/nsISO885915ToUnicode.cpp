




































#include "nsUCConstructors.h"
#include "nsISO885915ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-15.ut"
};

NS_METHOD
nsISO885915ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
