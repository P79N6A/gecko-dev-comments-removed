




































#include "nsUCConstructors.h"
#include "nsCP1256ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1256.ut"
};

NS_METHOD
nsCP1256ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

