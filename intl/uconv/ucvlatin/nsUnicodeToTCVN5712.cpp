




































#include "nsUCConstructors.h"
#include "nsUnicodeToTCVN5712.h"




static const PRUint16 g_ufMappingTable[] = {
#include "tcvn5712.uf"
};

NS_METHOD
nsUnicodeToTCVN5712Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
