




































#include "nsUCConstructors.h"
#include "nsUnicodeToISO88595.h"




static const PRUint16 g_ufMappingTable[] = {
#include "8859-5.uf"
};

NS_METHOD
nsUnicodeToISO88595Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

