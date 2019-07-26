




#include "nsUnicodeToHKSCS.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"







nsresult
nsUnicodeToHKSCSConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult)
{
  static const uint16_t *g_Big5HKSCSMappingTable[] = {
    g_ufBig5HKSCSMapping
  };

  static const uScanClassID g_Big5HKSCSScanClassIDs[] =  {
    u2BytesCharset
  };

  return CreateMultiTableEncoder(1,
                                 (uScanClassID*) &g_Big5HKSCSScanClassIDs,
                                 (uMappingTable**) &g_Big5HKSCSMappingTable,
                                 2 ,
                                 aOuter, aIID, aResult);
}

