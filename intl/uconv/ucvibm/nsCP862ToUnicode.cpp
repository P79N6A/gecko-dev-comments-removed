


















#include "nsUCConstructors.h"
#include "nsCP862ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp862.ut"
};

nsresult
nsCP862ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

