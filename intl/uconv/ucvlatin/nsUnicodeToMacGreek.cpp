




#include "nsUCConstructors.h"
#include "nsUnicodeToMacGreek.h"




nsresult
nsUnicodeToMacGreekConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_MacGreekMappingTable[] = {
#include "macgreek.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacGreekMappingTable, 1,
                            aOuter, aIID, aResult);
}
