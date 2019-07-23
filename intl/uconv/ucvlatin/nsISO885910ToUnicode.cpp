




































#include "nsUCConstructors.h"
#include "nsISO885910ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-10.ut"
};

NS_METHOD
nsISO885910ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

