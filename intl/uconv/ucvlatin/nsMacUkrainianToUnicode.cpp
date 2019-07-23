




































#include "nsUCConstructors.h"
#include "nsMacUkrainianToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macukrai.ut"
};

NS_METHOD
nsMacUkrainianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
