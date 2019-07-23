




































#include "nsUCConstructors.h"
#include "nsCP1046ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1046.ut"
};




NS_METHOD
nsCP1046ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
