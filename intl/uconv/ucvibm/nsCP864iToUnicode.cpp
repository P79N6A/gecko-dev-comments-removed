



















































#include "nsUCConstructors.h"
#include "nsCP864iToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "864i.ut"
};

nsresult
nsCP864iToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}



