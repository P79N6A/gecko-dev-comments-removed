





































#include "nsUCConstructors.h"
#include "nsGEOSTD8ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "geostd8.ut"
};

NS_METHOD
nsGEOSTD8ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                              void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

