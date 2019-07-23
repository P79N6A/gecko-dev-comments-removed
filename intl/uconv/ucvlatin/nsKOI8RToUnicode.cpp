




































#include "nsUCConstructors.h"
#include "nsKOI8RToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "koi8r.ut"
};

NS_METHOD
nsKOI8RToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

