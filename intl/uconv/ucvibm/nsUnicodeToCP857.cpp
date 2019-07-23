



















































#include "nsUCConstructors.h"
#include "nsUnicodeToCP857.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp857.uf"
};

NS_METHOD
nsUnicodeToCP857Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

