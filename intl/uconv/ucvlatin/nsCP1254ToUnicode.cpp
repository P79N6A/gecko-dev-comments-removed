




#include "nsUCConstructors.h"
#include "nsCP1254ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp1254.ut"
};

nsresult
nsCP1254ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
