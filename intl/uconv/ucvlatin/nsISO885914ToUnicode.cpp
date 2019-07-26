




#include "nsUCConstructors.h"
#include "nsISO885914ToUnicode.h"




nsresult
nsISO885914ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-14.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
