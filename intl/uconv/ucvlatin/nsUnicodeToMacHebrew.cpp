




#include "nsUCConstructors.h"
#include "nsUnicodeToMacHebrew.h"




nsresult
nsUnicodeToMacHebrewConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "machebrew.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
