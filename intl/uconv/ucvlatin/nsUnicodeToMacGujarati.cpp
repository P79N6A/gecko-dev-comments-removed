




#include "nsUCConstructors.h"
#include "nsUnicodeToMacGujarati.h"




nsresult
nsUnicodeToMacGujaratiConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "macgujarati.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
