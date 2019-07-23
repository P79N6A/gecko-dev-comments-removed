




































#include "nsUCConstructors.h"
#include "nsISOIR111ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "iso-ir-111.ut"
};

NS_METHOD
nsISOIR111ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
