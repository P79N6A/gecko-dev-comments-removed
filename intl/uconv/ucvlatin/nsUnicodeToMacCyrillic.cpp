




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacCyrillic.h"




static const PRUint16 g_ufMappingTable[] = {
#include "maccyril.uf"
};

NS_METHOD
nsUnicodeToMacCyrillicConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
