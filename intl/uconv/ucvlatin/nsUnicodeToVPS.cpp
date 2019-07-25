




#include "nsUCConstructors.h"
#include "nsUnicodeToVPS.h"




static const uint16_t g_ufMappingTable[] = {
#include "vps.uf"
};

nsresult
nsUnicodeToVPSConstructor(nsISupports *aOuter, REFNSIID aIID,
                          void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

