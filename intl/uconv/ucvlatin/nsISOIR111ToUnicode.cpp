




#include "nsUCConstructors.h"
#include "nsISOIR111ToUnicode.h"




nsresult
nsISOIR111ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "iso-ir-111.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
