




#include "nsUCConstructors.h"
#include "nsMacCEToUnicode.h"




nsresult
nsMacCEToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_MacCEMappingTable[] = {
#include "macce.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_MacCEMappingTable,
                            aOuter, aIID, aResult);
}

