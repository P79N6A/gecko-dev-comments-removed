




#include "nsUCConstructors.h"
#include "nsUnicodeToMacIcelandic.h"




nsresult
nsUnicodeToMacIcelandicConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "macicela.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

