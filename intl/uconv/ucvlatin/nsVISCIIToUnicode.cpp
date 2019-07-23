




































#include "nsVISCIIToUnicode.h"
#include "nsUCConstructors.h"




static const PRUint16 g_utMappingTable[] = {
#include "viscii.ut"
};

NS_METHOD
nsVISCIIToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
