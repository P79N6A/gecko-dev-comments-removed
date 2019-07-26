




#include "nsUCConstructors.h"
#include "nsTCVN5712ToUnicode.h"




nsresult
nsTCVN5712ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "tcvn5712.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
