




































#include "nsUCConstructors.h"
#include "nsMacCyrillicToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "maccyril.ut"
};

nsresult
nsMacCyrillicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

