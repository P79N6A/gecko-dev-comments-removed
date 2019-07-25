




































#include "nsUCConstructors.h"
#include "nsMacRomanToUnicode.h"




static const PRUint16 g_MacRomanMappingTable[] = {
#include "macroman.ut"
};

nsresult
nsMacRomanToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult)
{
  return CreateOneByteDecoder((uMappingTable*) &g_MacRomanMappingTable,
                            aOuter, aIID, aResult);
}

