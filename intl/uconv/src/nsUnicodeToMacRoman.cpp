




#include "nsUCConstructors.h"
#include "nsUnicodeToMacRoman.h"




nsresult
nsUnicodeToMacRomanConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult)
{
  static const uint16_t g_MacRomanMappingTable[] = {
#include "macroman.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacRomanMappingTable, 1,
                            aOuter, aIID, aResult);
}

