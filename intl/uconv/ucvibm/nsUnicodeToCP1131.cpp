




































#include "nsUCConstructors.h"
#include "nsUnicodeToCP1131.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp1131.uf"
};

NS_METHOD
nsUnicodeToCP1131Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset, 
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

