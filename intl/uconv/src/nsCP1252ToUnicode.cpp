




































#include "nsUCConstructors.h"
#include "nsCP1252ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1252.ut"
};

NS_METHOD
nsCP1252ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
