




































#include "nsUCConstructors.h"
#include "nsUnicodeToZapfDingbat.h"




static const PRUint16 g_ufMappingTable[] = {
#include "adobezingbat.uf"
};

NS_METHOD
nsUnicodeToZapfDingbatConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

