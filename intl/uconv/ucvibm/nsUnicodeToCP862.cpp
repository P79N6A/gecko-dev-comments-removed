


















#include "nsUCConstructors.h"
#include "nsUnicodeToCP862.h"




static const uint16_t g_ufMappingTable[] = {
#include "cp862.uf"
};

nsresult
nsUnicodeToCP862Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

