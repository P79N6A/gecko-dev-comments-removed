




#include "nsVISCIIToUnicode.h"
#include "nsUCConstructors.h"




static const uint16_t g_utMappingTable[] = {
#include "viscii.ut"
};

nsresult
nsVISCIIToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
