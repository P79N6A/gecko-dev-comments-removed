




































#include "nsUCConstructors.h"
#include "nsMacHebrewToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "machebrew.ut"
};

NS_METHOD
nsMacHebrewToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
