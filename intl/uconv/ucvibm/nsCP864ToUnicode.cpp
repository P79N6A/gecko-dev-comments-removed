



















































#include "nsUCConstructors.h"
#include "nsCP864ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp864.ut"
};

nsresult
nsCP864ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

