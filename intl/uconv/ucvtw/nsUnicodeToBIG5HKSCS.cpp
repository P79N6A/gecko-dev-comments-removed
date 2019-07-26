




#include "nsUnicodeToBIG5HKSCS.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"




nsresult
nsUnicodeToBIG5HKSCSConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult)
{
    static const uint16_t *g_Big5HKSCSMappingTable[] = {
      g_ASCIIMappingTable,
      g_ufBig5Mapping,
      g_ufBig5HKSCSMapping
    };

    static const uScanClassID g_Big5HKSCSScanClassIDs[] =  {
      u1ByteCharset,
      u2BytesCharset,
      u2BytesCharset
    };

    return CreateMultiTableEncoder(3,
                                   (uScanClassID*) &g_Big5HKSCSScanClassIDs,
                                   (uMappingTable**) &g_Big5HKSCSMappingTable,
                                   2 ,
                                   aOuter, aIID, aResult);
}


