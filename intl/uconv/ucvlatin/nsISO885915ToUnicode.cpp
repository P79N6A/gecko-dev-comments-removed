




#include "nsUCConstructors.h"
#include "nsISO885915ToUnicode.h"




nsresult
nsISO885915ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-15.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
