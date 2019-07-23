




































#include "nsUCConstructors.h"
#include "nsKOI8UToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "koi8u.ut"
};

NS_METHOD
nsKOI8UToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
