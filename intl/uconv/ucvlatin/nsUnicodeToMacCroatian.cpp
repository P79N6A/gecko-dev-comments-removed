




#include "nsUCConstructors.h"
#include "nsUnicodeToMacCroatian.h"




static const uint16_t g_ufMappingTable[] = {
#include "maccroat.uf"
};

nsresult
nsUnicodeToMacCroatianConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

