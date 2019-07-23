




































#include "nsUCConstructors.h"
#include "nsMacGujaratiToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macgujarati.ut"
};

NS_METHOD
nsMacGujaratiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
