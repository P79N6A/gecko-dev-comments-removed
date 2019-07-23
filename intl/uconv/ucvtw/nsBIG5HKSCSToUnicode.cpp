




































#include "nsBIG5HKSCSToUnicode.h"
#include "nsUCvTWDll.h"
#include "nsUCConstructors.h"




static const uScanClassID g_BIG5HKSCSScanClassIDs[] = {
  u1ByteCharset,
  u2BytesCharset,
  u2BytesCharset,
  u2BytesCharset,
  u2BytesCharset,
  u2BytesCharset
};

static const PRUint16 *g_BIG5HKSCSMappingTableSet [] ={
  g_ASCIIMapping,
  g_utBig5HKSCSMapping,
  g_utBIG5Mapping,
  g_utBig5HKSCSMapping,
  g_utBIG5Mapping,
  g_utBig5HKSCSMapping,
};

static const uRange g_BIG5HKSCSRanges[] = {
  { 0x00, 0x7E },
  { 0x81, 0xA0 },
  { 0xA1, 0xC6 },
  { 0xC6, 0xC8 },
  { 0xC9, 0xF9 },
  { 0xF9, 0xFE }
};




NS_METHOD
nsBIG5HKSCSToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult)
{
  return CreateMultiTableDecoder(6,
                                 (const uRange* ) &g_BIG5HKSCSRanges,
                                 (uScanClassID*) &g_BIG5HKSCSScanClassIDs,
                                 (uMappingTable**) &g_BIG5HKSCSMappingTableSet,
                                 1,
                                 aOuter, aIID, aResult);
}


