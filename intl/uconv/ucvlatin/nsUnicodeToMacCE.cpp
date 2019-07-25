




#include "nsUCConstructors.h"
#include "nsUnicodeToMacCE.h"




static const uint16_t g_MacCEMappingTable[] = {
#include "macce.uf"
};

nsresult
nsUnicodeToMacCEConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacCEMappingTable, 1,
                            aOuter, aIID, aResult);
}

