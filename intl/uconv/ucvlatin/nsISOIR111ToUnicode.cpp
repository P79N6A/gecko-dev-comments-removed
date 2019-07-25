




#include "nsUCConstructors.h"
#include "nsISOIR111ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "iso-ir-111.ut"
};

nsresult
nsISOIR111ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
