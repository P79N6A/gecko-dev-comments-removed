




#include "nsUnicodeToEUCKR.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"





static const uint16_t *g_EUCKRMappingTable[3] = {
  g_ucvko_AsciiMapping,
  g_ufKSC5601Mapping,
  g_HangulNullMapping
};

static const uScanClassID g_EUCKRScanCellIDTable[3] =  {
  u1ByteCharset,
  u2BytesGRCharset,
  uDecomposedHangulCharset
};

nsresult
nsUnicodeToEUCKRConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return CreateMultiTableEncoder(3,
                                 (uScanClassID*) g_EUCKRScanCellIDTable, 
                                 (uMappingTable**) g_EUCKRMappingTable,
                                 
                                 8 ,
                                 aOuter, aIID, aResult);
}

