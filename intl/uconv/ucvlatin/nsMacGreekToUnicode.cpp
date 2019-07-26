




#include "nsUCConstructors.h"
#include "nsMacGreekToUnicode.h"




nsresult
nsMacGreekToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_MacGreekMappingTable[] = {
#include "macgreek.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_MacGreekMappingTable,
                            aOuter, aIID, aResult);
}

