




#include "nsUCConstructors.h"
#include "nsUnicodeToCP874.h"




static const uint16_t g_ufMappingTable[] = {
#include "cp874.uf"
};

nsresult
nsUnicodeToCP874Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
