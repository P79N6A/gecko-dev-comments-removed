




#include "nsUCConstructors.h"
#include "nsUnicodeToVISCII.h"




nsresult
nsUnicodeToVISCIIConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "viscii.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

