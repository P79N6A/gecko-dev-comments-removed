




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacCE.h"




static const PRUint16 g_MacCEMappingTable[] = {
#include "macce.uf"
};

NS_METHOD
nsUnicodeToMacCEConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacCEMappingTable, 1,
                            aOuter, aIID, aResult);
}

