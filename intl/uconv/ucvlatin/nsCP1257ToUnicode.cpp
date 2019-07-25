




#include "nsUCConstructors.h"
#include "nsCP1257ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp1257.ut"
};

nsresult
nsCP1257ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
