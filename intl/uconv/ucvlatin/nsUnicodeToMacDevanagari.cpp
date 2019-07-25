




#include "nsUCConstructors.h"
#include "nsUnicodeToMacDevanagari.h"




static const uint16_t g_ufMappingTable[] = {
#include "macdevanaga.uf"
};

nsresult
nsUnicodeToMacDevanagariConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
