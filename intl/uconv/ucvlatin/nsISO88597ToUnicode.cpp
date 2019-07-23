




































#include "nsUCConstructors.h"
#include "nsISO88597ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-7.ut"
};

NS_METHOD
nsISO88597ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

