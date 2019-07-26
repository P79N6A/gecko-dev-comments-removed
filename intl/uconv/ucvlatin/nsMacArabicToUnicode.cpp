




#include "nsUCConstructors.h"
#include "nsMacArabicToUnicode.h"




nsresult
nsMacArabicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macarabic.ut"
   };

   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
