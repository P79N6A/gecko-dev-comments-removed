




#include "nsUCConstructors.h"
#include "nsISO885910ToUnicode.h"




nsresult
nsISO885910ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-10.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

