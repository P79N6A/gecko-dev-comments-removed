




































#include "nsUCConstructors.h"
#include "nsCP1250ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1250.ut"
};

nsresult
nsCP1250ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

