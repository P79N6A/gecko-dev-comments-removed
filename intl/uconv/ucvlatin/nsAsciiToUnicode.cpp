




































#include "nsUCConstructors.h"
#include "nsAsciiToUnicode.h"







static const PRUint16 g_utMappingTable[] = {
#include "cp1252.ut"
};

nsresult
nsAsciiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
