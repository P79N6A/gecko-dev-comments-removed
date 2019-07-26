




#include "nsUCConstructors.h"
#include "nsISO885916ToUnicode.h"




nsresult
nsISO885916ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-16.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
