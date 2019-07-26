




#include "nsUCConstructors.h"
#include "nsUnicodeToCP1254.h"




nsresult
nsUnicodeToCP1254Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "cp1254.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

