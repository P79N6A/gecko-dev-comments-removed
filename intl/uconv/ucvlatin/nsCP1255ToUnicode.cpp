




#include "nsUCConstructors.h"
#include "nsCP1255ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp1255.ut"
};

nsresult
nsCP1255ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
