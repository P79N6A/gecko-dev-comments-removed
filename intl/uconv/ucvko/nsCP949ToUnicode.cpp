





































#include "nsCP949ToUnicode.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"




static const uScanClassID g_CP949ScanClassIDs[] = {
  u1ByteCharset,




  uDecomposedHangulCharset,  
  u2BytesGRCharset,       
  u2BytesGR128Charset,    
  u2BytesCharset          
};


static const PRUint16 g_utCP949NoKSCHangulMapping[] = {
#include "u20cp949hangul.ut"
};

static const uRange g_CP949Ranges[] = {
  { 0x00, 0x7E },
  { 0xA4, 0xA4 },   
                    
  { 0xA1, 0xFE },
  { 0xA1, 0xC6 },   
  { 0x80, 0xA0 }
};

static const PRUint16 *g_CP949MappingTableSet [] ={
  g_ucvko_AsciiMapping,
  g_HangulNullMapping,
  g_utKSC5601Mapping,
  g_utCP949NoKSCHangulMapping,
  g_utCP949NoKSCHangulMapping


};


NS_METHOD
nsCP949ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return CreateMultiTableDecoder(sizeof(g_CP949Ranges) / sizeof(g_CP949Ranges[0]),
                                 (const uRange*) &g_CP949Ranges,
                                 (uScanClassID*) &g_CP949ScanClassIDs,
                                 (uMappingTable**) &g_CP949MappingTableSet, 1,
                                 aOuter, aIID, aResult);
}

