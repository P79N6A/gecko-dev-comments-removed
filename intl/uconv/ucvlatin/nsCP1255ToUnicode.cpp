




































#include "nsUCConstructors.h"
#include "nsCP1255ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1255.ut"
};

NS_METHOD
nsCP1255ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
