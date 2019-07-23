




































#include "nsUCConstructors.h"
#include "nsCP1251ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1251.ut"
};

NS_METHOD
nsCP1251ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
