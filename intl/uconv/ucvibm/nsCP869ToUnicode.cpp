



#include "nsUCConstructors.h"
#include "nsCP869ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp869.ut"
};

nsresult
nsCP869ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

