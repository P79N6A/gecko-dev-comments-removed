




































#include "nsUCConstructors.h"
#include "nsUnicodeToARMSCII8.h"




static const PRUint16 g_ufMappingTable[] = {
#include "armscii.uf"
};

NS_METHOD
nsUnicodeToARMSCII8Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

