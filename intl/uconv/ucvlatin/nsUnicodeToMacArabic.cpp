




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacArabic.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macarabic.uf"
};

NS_METHOD
nsUnicodeToMacArabicConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
