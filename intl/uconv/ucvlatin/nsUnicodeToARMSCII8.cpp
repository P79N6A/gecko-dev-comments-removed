




#include "nsUCConstructors.h"
#include "nsUnicodeToARMSCII8.h"




nsresult
nsUnicodeToARMSCII8Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "armscii.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

