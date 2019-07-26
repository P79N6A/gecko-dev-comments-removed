




#include "nsVISCIIToUnicode.h"
#include "nsUCConstructors.h"




nsresult
nsVISCIIToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "viscii.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
