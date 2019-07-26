




#include "nsUCConstructors.h"
#include "nsMacCyrillicToUnicode.h"




nsresult
nsMacCyrillicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "maccyril.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

