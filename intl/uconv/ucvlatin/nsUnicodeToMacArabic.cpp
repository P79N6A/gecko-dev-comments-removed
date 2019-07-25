




#include "nsUCConstructors.h"
#include "nsUnicodeToMacArabic.h"




static const uint16_t g_ufMappingTable[] = {
#include "macarabic.uf"
};

nsresult
nsUnicodeToMacArabicConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
