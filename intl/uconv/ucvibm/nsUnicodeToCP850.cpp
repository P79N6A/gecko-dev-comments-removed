


















#include "nsUCConstructors.h"
#include "nsUnicodeToCP850.h"




static const uint16_t g_ufMappingTable[] = {
#include "cp850.uf"
};

nsresult
nsUnicodeToCP850Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset, 
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

