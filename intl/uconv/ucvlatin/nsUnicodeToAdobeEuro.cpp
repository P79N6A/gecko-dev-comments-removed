




































#include "nsUCConstructors.h"
#include "nsUnicodeToAdobeEuro.h"




static const PRUint16 g_ufMappingTable[] = {



#include "adobeeuro.uf"
};

NS_METHOD
nsUnicodeToAdobeEuroConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
