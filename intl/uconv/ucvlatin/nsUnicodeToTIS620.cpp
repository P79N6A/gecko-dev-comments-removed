




#include "nsUCConstructors.h"
#include "nsUnicodeToTIS620.h"




static const uint16_t g_ufMappingTable[] = {
#include "tis620.uf"
};

nsresult
nsUnicodeToTIS620Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

