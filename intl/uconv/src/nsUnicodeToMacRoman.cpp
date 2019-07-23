




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacRoman.h"




static const PRUint16 g_MacRomanMappingTable[] = {
#include "macroman.uf"
};

NS_METHOD
nsUnicodeToMacRomanConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult)
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacRomanMappingTable, 1,
                            aOuter, aIID, aResult);
}

