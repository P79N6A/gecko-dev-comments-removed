




#include "nsUCConstructors.h"
#include "nsCP1256ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp1256.ut"
};

nsresult
nsCP1256ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

