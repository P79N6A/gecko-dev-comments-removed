




































#include "nsUCConstructors.h"
#include "nsCP1258ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1258.ut"
};

nsresult
nsCP1258ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

