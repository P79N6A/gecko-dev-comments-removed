




































#include "nsUCConstructors.h"
#include "nsCP1253ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1253.ut"
};

NS_METHOD
nsCP1253ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

