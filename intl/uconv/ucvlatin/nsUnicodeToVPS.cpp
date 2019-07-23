




































#include "nsUCConstructors.h"
#include "nsUnicodeToVPS.h"




static const PRUint16 g_ufMappingTable[] = {
#include "vps.uf"
};

NS_METHOD
nsUnicodeToVPSConstructor(nsISupports *aOuter, REFNSIID aIID,
                          void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

