




































#include "nsUnicodeToHKSCS.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"






static const PRUint16 *g_Big5HKSCSMappingTable[] = {
  g_ufBig5HKSCSMapping
};

static const uScanClassID g_Big5HKSCSScanClassIDs[] =  {
  u2BytesCharset
};




NS_METHOD
nsUnicodeToHKSCSConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult)
{
  return CreateMultiTableEncoder(1,
                                 (uScanClassID*) &g_Big5HKSCSScanClassIDs,
                                 (uMappingTable**) &g_Big5HKSCSMappingTable,
                                 2 ,
                                 aOuter, aIID, aResult);
}

