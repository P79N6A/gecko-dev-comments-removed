




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacCroatian.h"




static const PRUint16 g_ufMappingTable[] = {
#include "maccroat.uf"
};

NS_METHOD
nsUnicodeToMacCroatianConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

