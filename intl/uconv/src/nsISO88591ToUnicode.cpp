




































#include "nsUCConstructors.h"
#include "nsISO88591ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp1252.ut"
};

nsresult
nsISO88591ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

