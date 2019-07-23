




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacRomanian.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macro.uf"
};

NS_METHOD
nsUnicodeToMacRomanianConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
