





































#include "nsUCConstructors.h"
#include "nsUnicodeToGEOSTD8.h"




static const PRUint16 g_ufMappingTable[] = {
#include "geostd8.uf"
};

NS_METHOD
nsUnicodeToGEOSTD8Constructor(nsISupports *aOuter, REFNSIID aIID,
                              void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
