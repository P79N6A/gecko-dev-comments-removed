




































#include "nsUCConstructors.h"
#include "nsMacRomanianToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macro.ut"
};

NS_METHOD
nsMacRomanianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
