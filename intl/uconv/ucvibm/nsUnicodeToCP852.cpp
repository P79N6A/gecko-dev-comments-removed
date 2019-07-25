


















#include "nsUCConstructors.h"
#include "nsUnicodeToCP852.h"




static const uint16_t g_ufMappingTable[] = {
#include "cp852.uf"
};

nsresult
nsUnicodeToCP852Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

