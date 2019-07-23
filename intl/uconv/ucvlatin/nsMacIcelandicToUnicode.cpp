




































#include "nsUCConstructors.h"
#include "nsMacIcelandicToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macicela.ut"
};

NS_METHOD
nsMacIcelandicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
