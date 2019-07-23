




































#include "nsUCConstructors.h"
#include "nsMacTurkishToUnicode.h"




static const PRUint16 g_MacTurkishMappingTable[] = {
#include "macturki.ut"
};

NS_METHOD
nsMacTurkishToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_MacTurkishMappingTable,
                            aOuter, aIID, aResult);
}
