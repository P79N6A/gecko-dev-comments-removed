




































#include "nsUCConstructors.h"
#include "nsUnicodeToSymbol.h"




static const PRUint16 g_ufMappingTable[] = {
#include "adobesymbol.uf"
};

NS_METHOD
nsUnicodeToSymbolConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
