




































#include "nsUCConstructors.h"
#include "nsUnicodeToCP1250.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp1250.uf"
};

NS_METHOD
nsUnicodeToCP1250Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

