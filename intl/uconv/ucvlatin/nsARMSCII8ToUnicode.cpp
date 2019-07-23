




































#include "nsUCConstructors.h"
#include "nsARMSCII8ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "armscii.ut"
};

NS_METHOD
nsARMSCII8ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

