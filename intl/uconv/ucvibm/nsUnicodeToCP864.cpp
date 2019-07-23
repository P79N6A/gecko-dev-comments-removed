



















































#include "nsUCConstructors.h"
#include "nsUnicodeToCP864.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp864.uf"
};

NS_METHOD
nsUnicodeToCP864Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

