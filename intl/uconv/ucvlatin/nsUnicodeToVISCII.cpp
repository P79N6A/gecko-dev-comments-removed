




































#include "nsUCConstructors.h"
#include "nsUnicodeToVISCII.h"




static const PRUint16 g_ufMappingTable[] = {
#include "viscii.uf"
};

NS_METHOD
nsUnicodeToVISCIIConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

