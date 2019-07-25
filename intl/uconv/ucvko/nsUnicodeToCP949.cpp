





#include "nsUnicodeToCP949.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"






static const uint16_t g_ufCP949NoKSCHangulMapping[] = {
#include "u20cp949hangul.uf"
};



static const uint16_t *g_CP949MappingTable[3] = {
  g_ucvko_AsciiMapping,
  g_ufKSC5601Mapping,
  g_ufCP949NoKSCHangulMapping
};

static const uScanClassID g_CP949ScanClassTable[3] =  {
  u1ByteCharset,
  u2BytesGRCharset,
  u2BytesCharset
};

nsresult
nsUnicodeToCP949Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return CreateMultiTableEncoder(3,
                                 (uScanClassID*) g_CP949ScanClassTable, 
                                 (uMappingTable**) g_CP949MappingTable,
                                 2 ,
                                 aOuter, aIID, aResult);
}

