




































#include "nsUCConstructors.h"
#include "nsUnicodeToCP1253.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp1253.uf"
};

NS_METHOD
nsUnicodeToCP1253Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

