




#include "nsUCConstructors.h"
#include "nsCP1254ToUnicode.h"




nsresult
nsCP1254ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp1254.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
