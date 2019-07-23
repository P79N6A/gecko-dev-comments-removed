




































#include "nsUCConstructors.h"
#include "nsUnicodeToCP874.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp874.uf"
};

NS_METHOD
nsUnicodeToCP874Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
