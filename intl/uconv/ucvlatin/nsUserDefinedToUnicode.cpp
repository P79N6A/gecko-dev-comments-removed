




































#include "nsUCConstructors.h"
#include "nsUserDefinedToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "userdefined.ut"
};

NS_METHOD
nsUserDefinedToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
