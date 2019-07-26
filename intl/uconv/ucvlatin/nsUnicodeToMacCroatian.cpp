




#include "nsUCConstructors.h"
#include "nsUnicodeToMacCroatian.h"




nsresult
nsUnicodeToMacCroatianConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "maccroat.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

