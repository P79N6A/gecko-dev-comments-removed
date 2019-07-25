




#include "nsUCConstructors.h"
#include "nsMacGujaratiToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "macgujarati.ut"
};

nsresult
nsMacGujaratiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
