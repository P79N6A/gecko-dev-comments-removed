




#include "nsUCConstructors.h"
#include "nsMacFarsiToUnicode.h"




nsresult
nsMacFarsiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macfarsi.ut"
   };

   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
