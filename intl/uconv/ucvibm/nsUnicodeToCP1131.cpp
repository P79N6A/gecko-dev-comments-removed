



#include "nsUCConstructors.h"
#include "nsUnicodeToCP1131.h"




static const uint16_t g_ufMappingTable[] = {
#include "cp1131.uf"
};

nsresult
nsUnicodeToCP1131Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset, 
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

