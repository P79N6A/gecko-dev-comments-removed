




#include "nsUCConstructors.h"
#include "nsUnicodeToMacFarsi.h"




nsresult
nsUnicodeToMacFarsiConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "macfarsi.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
