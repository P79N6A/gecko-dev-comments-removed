




































#include "nsUCConstructors.h"
#include "nsCP874ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp874.ut"
};

NS_METHOD
nsCP874ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
