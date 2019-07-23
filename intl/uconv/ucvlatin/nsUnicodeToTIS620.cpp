






































#include "nsUCConstructors.h"
#include "nsUnicodeToTIS620.h"




static const PRUint16 g_ufMappingTable[] = {
#include "tis620.uf"
};

NS_METHOD
nsUnicodeToTIS620Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

