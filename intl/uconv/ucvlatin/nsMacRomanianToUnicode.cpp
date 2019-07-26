




#include "nsUCConstructors.h"
#include "nsMacRomanianToUnicode.h"




nsresult
nsMacRomanianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "macro.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
