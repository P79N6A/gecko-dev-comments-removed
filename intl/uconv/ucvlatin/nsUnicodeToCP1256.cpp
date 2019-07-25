




#include "nsUCConstructors.h"
#include "nsUnicodeToCP1256.h"




static const uint16_t g_ufMappingTable[] = {
#include "cp1256.uf"
};

nsresult
nsUnicodeToCP1256Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

