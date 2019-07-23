




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacGurmukhi.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macgurmukhi.uf"
};

NS_METHOD
nsUnicodeToMacGurmukhiConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

