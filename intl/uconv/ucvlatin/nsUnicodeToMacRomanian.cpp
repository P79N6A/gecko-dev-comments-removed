




#include "nsUCConstructors.h"
#include "nsUnicodeToMacRomanian.h"




nsresult
nsUnicodeToMacRomanianConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "macro.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
