



















































#include "nsUCConstructors.h"
#include "nsUnicodeToCP855.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp855.uf"
};

NS_METHOD
nsUnicodeToCP855Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

