




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacIcelandic.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macicela.uf"
};

NS_METHOD
nsUnicodeToMacIcelandicConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

