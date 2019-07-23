



















































#include "nsUCConstructors.h"
#include "nsUnicodeToCP852.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp852.uf"
};

NS_METHOD
nsUnicodeToCP852Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

