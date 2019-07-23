




































#include "nsUCConstructors.h"
#include "nsUnicodeToKOI8U.h"




static const PRUint16 g_ufMappingTable[] = {
#include "koi8u.uf"
};

NS_METHOD
nsUnicodeToKOI8UConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

