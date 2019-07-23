




































#include "nsUCConstructors.h"
#include "nsCP1254ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1254.ut"
};

NS_METHOD
nsCP1254ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
