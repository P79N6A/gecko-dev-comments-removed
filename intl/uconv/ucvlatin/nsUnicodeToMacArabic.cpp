




#include "nsUCConstructors.h"
#include "nsUnicodeToMacArabic.h"




nsresult
nsUnicodeToMacArabicConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "macarabic.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
