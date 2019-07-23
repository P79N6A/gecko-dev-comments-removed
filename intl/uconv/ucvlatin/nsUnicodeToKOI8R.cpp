




































#include "nsUCConstructors.h"
#include "nsUnicodeToKOI8R.h"




static const PRUint16 g_ufMappingTable[] = {
#include "koi8r.uf"
};

NS_METHOD
nsUnicodeToKOI8RConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
