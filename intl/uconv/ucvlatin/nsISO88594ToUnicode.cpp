




#include "nsUCConstructors.h"
#include "nsISO88594ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "8859-4.ut"
};

nsresult
nsISO88594ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}


