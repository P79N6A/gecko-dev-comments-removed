




#include "nsUCConstructors.h"
#include "nsMacGurmukhiToUnicode.h"




nsresult
nsMacGurmukhiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macgurmukhi.ut"
   };

   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
