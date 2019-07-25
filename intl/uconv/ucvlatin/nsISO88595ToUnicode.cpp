




#include "nsUCConstructors.h"
#include "nsISO88595ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "8859-5.ut"
};

nsresult
nsISO88595ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

