




































#include "nsUCConstructors.h"
#include "nsCP1257ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1257.ut"
};

NS_METHOD
nsCP1257ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
