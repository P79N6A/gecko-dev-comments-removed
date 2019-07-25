




































#include "nsUCConstructors.h"
#include "nsMacCroatianToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "maccroat.ut"
};

nsresult
nsMacCroatianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

