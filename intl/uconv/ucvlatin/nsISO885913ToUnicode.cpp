




































#include "nsUCConstructors.h"
#include "nsISO885913ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "8859-13.ut"
};

NS_METHOD
nsISO885913ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

