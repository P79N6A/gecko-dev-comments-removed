




#include "nsUCConstructors.h"
#include "nsMacHebrewToUnicode.h"




nsresult
nsMacHebrewToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "machebrew.ut"
   };

   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
