




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacGreek.h"




static const PRUint16 g_MacGreekMappingTable[] = {
#include "macgreek.uf"
};

NS_METHOD
nsUnicodeToMacGreekConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacGreekMappingTable, 1,
                            aOuter, aIID, aResult);
}
