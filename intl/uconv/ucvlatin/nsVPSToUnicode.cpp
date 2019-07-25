




































#include "nsVPSToUnicode.h"
#include "nsUCConstructors.h"




static const PRUint16 g_utMappingTable[] = {
#include "vps.ut"
};

nsresult
nsVPSToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                          void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

