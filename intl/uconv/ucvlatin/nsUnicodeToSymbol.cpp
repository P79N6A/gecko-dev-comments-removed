




#include "nsUCConstructors.h"
#include "nsUnicodeToSymbol.h"




nsresult
nsUnicodeToSymbolConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "adobesymbol.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
