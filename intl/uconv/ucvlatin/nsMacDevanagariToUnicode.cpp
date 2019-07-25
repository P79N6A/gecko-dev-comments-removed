




#include "nsUCConstructors.h"
#include "nsMacDevanagariToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "macdevanaga.ut"
};

nsresult
nsMacDevanagariToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
