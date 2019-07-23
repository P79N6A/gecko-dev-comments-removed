




































#include "nsUnicodeToBIG5HKSCS.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"





static const PRUint16 *g_Big5HKSCSMappingTable[] = {
  g_ASCIIMapping,
  g_ufBig5Mapping,
  g_ufBig5HKSCSMapping
};

static const uScanClassID g_Big5HKSCSScanClassIDs[] =  {
  u1ByteCharset,
  u2BytesCharset,
  u2BytesCharset
};

NS_METHOD
nsUnicodeToBIG5HKSCSConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult)
{
    return CreateMultiTableEncoder(3,
                                   (uScanClassID*) &g_Big5HKSCSScanClassIDs,
                                   (uMappingTable**) &g_Big5HKSCSMappingTable,
                                   2 ,
                                   aOuter, aIID, aResult);
}


