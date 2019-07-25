




#include "nsUCConstructors.h"
#include "nsMacGurmukhiToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "macgurmukhi.ut"
};

nsresult
nsMacGurmukhiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
