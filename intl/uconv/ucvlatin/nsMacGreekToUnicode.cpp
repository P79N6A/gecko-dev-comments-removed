




#include "nsUCConstructors.h"
#include "nsMacGreekToUnicode.h"




static const uint16_t g_MacGreekMappingTable[] = {
#include "macgreek.ut"
};

nsresult
nsMacGreekToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_MacGreekMappingTable,
                            aOuter, aIID, aResult);
}

