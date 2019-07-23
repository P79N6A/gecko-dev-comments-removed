





































#include "nsJohabToUnicode.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"




static const uScanClassID g_JOHABScanClassIDs[] = {
  u1ByteCharset,
  uJohabHangulCharset,
  u2BytesCharset,
  uJohabSymbolCharset,
  uJohabSymbolCharset
};

static const uRange g_JOHABRanges[] = {
  { 0x00, 0x7E },
  { 0x84, 0xD3 },
  { 0x84, 0xD3 },
  { 0xD8, 0xDE },
  { 0xE0, 0xF9 }
};

static const PRUint16 g_utJohabJamoMapping[] ={   
#include "johabjamo.ut"
};

static const PRUint16 *g_JOHABMappingTableSet [] ={
  g_ucvko_AsciiMapping,
  g_HangulNullMapping,
  g_utJohabJamoMapping,
  g_utKSC5601Mapping,
  g_utKSC5601Mapping
};





NS_METHOD
nsJohabToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return CreateMultiTableDecoder(sizeof(g_JOHABRanges) / sizeof(g_JOHABRanges[0]),
                                 (const uRange*) &g_JOHABRanges,
                                 (uScanClassID*) &g_JOHABScanClassIDs,
                                 (uMappingTable**) &g_JOHABMappingTableSet, 1,
                                 aOuter, aIID, aResult);
}

