



















































#include "nsUCConstructors.h"
#include "nsCP855ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp855.ut"
};

NS_METHOD
nsCP855ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

