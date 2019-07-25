


















#include "nsUCConstructors.h"
#include "nsUnicodeToCP857.h"




static const uint16_t g_ufMappingTable[] = {
#include "cp857.uf"
};

nsresult
nsUnicodeToCP857Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

