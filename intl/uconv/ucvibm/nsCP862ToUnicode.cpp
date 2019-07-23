



















































#include "nsUCConstructors.h"
#include "nsCP862ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp862.ut"
};

NS_METHOD
nsCP862ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

