




#include "nsUCConstructors.h"
#include "nsUnicodeToUserDefined.h"




static const uint16_t g_ufMappingTable[] = {
#include "userdefined.uf"
};

nsresult
nsUnicodeToUserDefinedConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
