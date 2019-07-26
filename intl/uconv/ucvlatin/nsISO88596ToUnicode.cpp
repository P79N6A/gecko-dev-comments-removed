




#include "nsUCConstructors.h"
#include "nsISO88596ToUnicode.h"




nsresult
nsISO88596ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-6.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

