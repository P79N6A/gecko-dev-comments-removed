




#include "nsVPSToUnicode.h"
#include "nsUCConstructors.h"




nsresult
nsVPSToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                          void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "vps.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

