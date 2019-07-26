




#include "nsUCConstructors.h"
#include "nsCP1255ToUnicode.h"




nsresult
nsCP1255ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp1255.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
