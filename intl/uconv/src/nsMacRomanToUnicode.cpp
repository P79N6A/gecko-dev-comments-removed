




#include "nsUCConstructors.h"
#include "nsMacRomanToUnicode.h"




static const uint16_t g_MacRomanMappingTable[] = {
#include "macroman.ut"
};

nsresult
nsMacRomanToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult)
{
  return CreateOneByteDecoder((uMappingTable*) &g_MacRomanMappingTable,
                            aOuter, aIID, aResult);
}

