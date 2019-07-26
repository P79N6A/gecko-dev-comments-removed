




#include "nsUCConstructors.h"
#include "nsUnicodeToMacGurmukhi.h"




nsresult
nsUnicodeToMacGurmukhiConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "macgurmukhi.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

