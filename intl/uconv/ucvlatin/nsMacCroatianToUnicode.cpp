




#include "nsUCConstructors.h"
#include "nsMacCroatianToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "maccroat.ut"
};

nsresult
nsMacCroatianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

