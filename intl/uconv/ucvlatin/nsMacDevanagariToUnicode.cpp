




#include "nsUCConstructors.h"
#include "nsMacDevanagariToUnicode.h"




nsresult
nsMacDevanagariToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macdevanaga.ut"
   };

   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
