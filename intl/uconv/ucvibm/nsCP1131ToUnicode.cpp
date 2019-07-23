




































#include "nsUCConstructors.h"
#include "nsCP1131ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1131.ut"
};




NS_METHOD
nsCP1131ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
