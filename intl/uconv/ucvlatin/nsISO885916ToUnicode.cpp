




#include "nsUCConstructors.h"
#include "nsISO885916ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "8859-16.ut"
};

nsresult
nsISO885916ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
