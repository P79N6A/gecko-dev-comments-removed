




































#include "nsUCConstructors.h"
#include "nsUnicodeToISOIR111.h"




static const PRUint16 g_ufMappingTable[] = {
#include "iso-ir-111.uf"
};

NS_METHOD
nsUnicodeToISOIR111Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
