




#include "nsUCConstructors.h"
#include "nsMacRomanToUnicode.h"




nsresult
nsMacRomanToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult)
{
  static const uint16_t g_MacRomanMappingTable[] = {
#include "macroman.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_MacRomanMappingTable,
                            aOuter, aIID, aResult);
}

