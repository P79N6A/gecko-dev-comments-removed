




#include "nsUCConstructors.h"
#include "nsISO88591ToUnicode.h"




nsresult
nsISO88591ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp1252.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

