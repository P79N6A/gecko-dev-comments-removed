




































#include "nsBIG5ToUnicode.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"




static const uScanClassID g_BIG5ScanClassIDs[] = {
  u1ByteCharset,
  u2BytesCharset
};

static const PRUint16 *g_BIG5MappingTableSet [] ={
  g_ASCIIMapping,
  g_utBIG5Mapping
};

static const uRange g_BIG5Ranges[] = {
  { 0x00, 0x7E },
  { 0x81, 0xFE }
};

NS_METHOD
nsBIG5ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                           void **aResult)
{
  return CreateMultiTableDecoder(2, 
                                 (const uRange* ) &g_BIG5Ranges,
                                 (uScanClassID*) &g_BIG5ScanClassIDs,
                                 (uMappingTable**) &g_BIG5MappingTableSet, 1,
                                 aOuter, aIID, aResult);
}


