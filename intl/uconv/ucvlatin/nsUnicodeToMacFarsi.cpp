




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacFarsi.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macfarsi.uf"
};

NS_METHOD
nsUnicodeToMacFarsiConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
