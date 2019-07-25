




#include "nsUCConstructors.h"
#include "nsMacRomanianToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "macro.ut"
};

nsresult
nsMacRomanianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
