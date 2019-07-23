




































#include "nsUCConstructors.h"
#include "nsMacCroatianToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "maccroat.ut"
};

NS_METHOD
nsMacCroatianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

