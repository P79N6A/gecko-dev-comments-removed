




































#include "nsUCConstructors.h"
#include "nsMacDevanagariToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macdevanaga.ut"
};

NS_METHOD
nsMacDevanagariToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
