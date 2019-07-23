





































#include "nsUnicodeToJohabNoAscii.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"






static const PRUint16 *g_JohabMappingTable[3] = {
  g_HangulNullMapping,
  g_ufJohabJamoMapping,
  g_ufKSC5601Mapping
};

static const uScanClassID g_JohabScanClassTable[3] =  {
  uJohabHangulCharset,
  u2BytesCharset,
  uJohabSymbolCharset
};

NS_METHOD
nsUnicodeToJohabNoAsciiConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult)
{
  return CreateMultiTableEncoder(sizeof(g_JohabScanClassTable) / sizeof(g_JohabScanClassTable[0]),
                                 (uScanClassID*) g_JohabScanClassTable, 
                                 (uMappingTable**) g_JohabMappingTable,
                                 2 ,
                                 aOuter, aIID, aResult);
}
