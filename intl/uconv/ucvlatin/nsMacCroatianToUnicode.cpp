




#include "nsUCConstructors.h"
#include "nsMacCroatianToUnicode.h"




nsresult
nsMacCroatianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "maccroat.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

