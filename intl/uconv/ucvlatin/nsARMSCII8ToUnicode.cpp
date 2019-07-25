




#include "nsUCConstructors.h"
#include "nsARMSCII8ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "armscii.ut"
};

nsresult
nsARMSCII8ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

