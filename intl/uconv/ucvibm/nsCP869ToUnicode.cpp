



































#include "nsUCConstructors.h"
#include "nsCP869ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp869.ut"
};

NS_METHOD
nsCP869ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

