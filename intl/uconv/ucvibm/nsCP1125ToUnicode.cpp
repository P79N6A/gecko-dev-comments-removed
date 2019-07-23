




































#include "nsUCConstructors.h"
#include "nsCP1125ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1125.ut"
};




NS_METHOD
nsCP1125ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
