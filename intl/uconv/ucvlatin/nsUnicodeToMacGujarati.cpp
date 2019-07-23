




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacGujarati.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macgujarati.uf"
};

NS_METHOD
nsUnicodeToMacGujaratiConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
