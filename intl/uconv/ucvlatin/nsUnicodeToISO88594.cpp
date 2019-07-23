




































#include "nsUCConstructors.h"
#include "nsUnicodeToISO88594.h"




static const PRUint16 g_ufMappingTable[] = {
#include "8859-4.uf"
};

NS_METHOD
nsUnicodeToISO88594Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

