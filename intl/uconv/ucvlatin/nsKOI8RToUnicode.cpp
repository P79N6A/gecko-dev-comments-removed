




#include "nsUCConstructors.h"
#include "nsKOI8RToUnicode.h"




nsresult
nsKOI8RToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "koi8r.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

