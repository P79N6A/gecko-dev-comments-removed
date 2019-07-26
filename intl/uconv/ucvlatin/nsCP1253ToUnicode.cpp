




#include "nsUCConstructors.h"
#include "nsCP1253ToUnicode.h"




nsresult
nsCP1253ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp1253.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

