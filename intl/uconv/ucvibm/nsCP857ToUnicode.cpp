


















#include "nsUCConstructors.h"
#include "nsCP857ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp857.ut"
};

nsresult
nsCP857ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

