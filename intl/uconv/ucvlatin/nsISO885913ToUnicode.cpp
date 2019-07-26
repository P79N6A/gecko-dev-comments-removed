




#include "nsUCConstructors.h"
#include "nsISO885913ToUnicode.h"




nsresult
nsISO885913ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-13.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

