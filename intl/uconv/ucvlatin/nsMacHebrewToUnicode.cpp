




#include "nsUCConstructors.h"
#include "nsMacHebrewToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "machebrew.ut"
};

nsresult
nsMacHebrewToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
