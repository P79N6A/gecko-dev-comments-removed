




#include "nsUCConstructors.h"
#include "nsUnicodeToTCVN5712.h"




nsresult
nsUnicodeToTCVN5712Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "tcvn5712.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
