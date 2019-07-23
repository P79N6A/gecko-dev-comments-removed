




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacUkrainian.h"




static const PRUint16 g_ufMappingTable[] = {
#include "macukrai.uf"
};

NS_METHOD
nsUnicodeToMacUkrainianConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

