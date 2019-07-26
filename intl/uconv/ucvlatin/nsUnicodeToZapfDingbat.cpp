




#include "nsUCConstructors.h"
#include "nsUnicodeToZapfDingbat.h"




nsresult
nsUnicodeToZapfDingbatConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "adobezingbat.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

