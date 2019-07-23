




































#include "nsUCConstructors.h"
#include "nsUnicodeToUserDefined.h"




static const PRUint16 g_ufMappingTable[] = {
#include "userdefined.uf"
};

NS_METHOD
nsUnicodeToUserDefinedConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
