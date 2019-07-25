




































#include "nsUCConstructors.h"
#include "nsMacArabicToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macarabic.ut"
};

nsresult
nsMacArabicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
