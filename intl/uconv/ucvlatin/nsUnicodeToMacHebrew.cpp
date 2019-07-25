




#include "nsUCConstructors.h"
#include "nsUnicodeToMacHebrew.h"




static const uint16_t g_ufMappingTable[] = {
#include "machebrew.uf"
};

nsresult
nsUnicodeToMacHebrewConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
