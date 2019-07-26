




#include "nsUCConstructors.h"
#include "nsISO88598ToUnicode.h"




nsresult
nsISO88598ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-8.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

