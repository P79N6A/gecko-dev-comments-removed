




#include "nsUCConstructors.h"
#include "nsUnicodeToISOIR111.h"




nsresult
nsUnicodeToISOIR111Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "iso-ir-111.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
