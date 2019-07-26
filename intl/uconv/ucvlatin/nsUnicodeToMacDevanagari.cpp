




#include "nsUCConstructors.h"
#include "nsUnicodeToMacDevanagari.h"




nsresult
nsUnicodeToMacDevanagariConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "macdevanaga.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
