


















#include "nsUCConstructors.h"
#include "nsCP855ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp855.ut"
};

nsresult
nsCP855ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

