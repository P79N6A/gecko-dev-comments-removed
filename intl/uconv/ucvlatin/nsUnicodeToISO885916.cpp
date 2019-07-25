




#include "nsUCConstructors.h"
#include "nsUnicodeToISO885916.h"




static const uint16_t g_ufMappingTable[] = {
#include "8859-16.uf"
};

nsresult
nsUnicodeToISO885916Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

