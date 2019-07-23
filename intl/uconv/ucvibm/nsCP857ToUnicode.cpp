



















































#include "nsUCConstructors.h"
#include "nsCP857ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp857.ut"
};

NS_METHOD
nsCP857ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

