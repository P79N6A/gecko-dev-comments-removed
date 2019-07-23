




































#include "nsUCConstructors.h"
#include "nsMacGurmukhiToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "macgurmukhi.ut"
};

NS_METHOD
nsMacGurmukhiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
