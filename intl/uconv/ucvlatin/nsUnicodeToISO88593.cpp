




































#include "nsUCConstructors.h"
#include "nsUnicodeToISO88593.h"




static const PRUint16 g_ufMappingTable[] = {
#include "8859-3.uf"
};

NS_METHOD
nsUnicodeToISO88593Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
