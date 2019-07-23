




































#include "nsUCConstructors.h"
#include "nsMacGreekToUnicode.h"




static const PRUint16 g_MacGreekMappingTable[] = {
#include "macgreek.ut"
};

NS_METHOD
nsMacGreekToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_MacGreekMappingTable,
                            aOuter, aIID, aResult);
}

