




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacDevanagari.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macdevanaga.uf"
};

NS_METHOD
nsUnicodeToMacDevanagariConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
