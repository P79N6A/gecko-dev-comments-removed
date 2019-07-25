




































#include "nsUCConstructors.h"
#include "nsMacFarsiToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macfarsi.ut"
};

nsresult
nsMacFarsiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
