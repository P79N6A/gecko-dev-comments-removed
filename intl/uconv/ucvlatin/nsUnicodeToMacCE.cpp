




#include "nsUCConstructors.h"
#include "nsUnicodeToMacCE.h"




nsresult
nsUnicodeToMacCEConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_MacCEMappingTable[] = {
#include "macce.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacCEMappingTable, 1,
                            aOuter, aIID, aResult);
}

