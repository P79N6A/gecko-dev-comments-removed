




































#include "nsUCConstructors.h"
#include "nsMacCEToUnicode.h"




static const PRUint16 g_MacCEMappingTable[] = {
#include "macce.ut"
};

NS_METHOD
nsMacCEToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_MacCEMappingTable,
                            aOuter, aIID, aResult);
}

