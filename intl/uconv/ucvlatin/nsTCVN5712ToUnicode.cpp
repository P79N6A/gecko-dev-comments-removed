




#include "nsUCConstructors.h"
#include "nsTCVN5712ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "tcvn5712.ut"
};

nsresult
nsTCVN5712ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
