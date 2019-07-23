



















































#include "nsUCConstructors.h"
#include "nsUnicodeToCP862.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp862.uf"
};

NS_METHOD
nsUnicodeToCP862Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

