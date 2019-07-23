




































#include "nsUnicodeToBIG5.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"





static const PRUint16 *g_Big5MappingTable[2] = {
  g_ASCIIMapping,
  g_ufBig5Mapping
};

static const uScanClassID g_Big5ScanClassIDs[2] =  {
  u1ByteCharset,
  u2BytesCharset
};




NS_METHOD
nsUnicodeToBIG5Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult)
{

  return CreateMultiTableEncoder(2,
                                 (uScanClassID*) &g_Big5ScanClassIDs,
                                 (uMappingTable**) &g_Big5MappingTable,
                                 2 ,
                                 aOuter, aIID, aResult);
}

