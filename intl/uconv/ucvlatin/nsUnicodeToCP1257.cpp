




































#include "nsUCConstructors.h"
#include "nsUnicodeToCP1257.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp1257.uf"
};

NS_METHOD
nsUnicodeToCP1257Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

