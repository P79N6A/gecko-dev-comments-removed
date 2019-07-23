




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacHebrew.h"




static const PRUint16 g_ufMappingTable[] = {
#include "machebrew.uf"
};

NS_METHOD
nsUnicodeToMacHebrewConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
