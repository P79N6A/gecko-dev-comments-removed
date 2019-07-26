




#include "nsUnicodeToBIG5.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"





static const uint16_t *g_Big5MappingTable[2] = {
  g_ASCIIMappingTable,
  g_ufBig5Mapping
};

static const uScanClassID g_Big5ScanClassIDs[2] =  {
  u1ByteCharset,
  u2BytesCharset
};




nsresult
nsUnicodeToBIG5Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult)
{

  return CreateMultiTableEncoder(2,
                                 (uScanClassID*) &g_Big5ScanClassIDs,
                                 (uMappingTable**) &g_Big5MappingTable,
                                 2 ,
                                 aOuter, aIID, aResult);
}

