




































#include "nsUnicodeToEUCTW.h"
#include "nsUCvTW2Dll.h"
#include "nsUCConstructors.h"




static const uScanClassID g_EUCTWScanClassSet [] = {
  u1ByteCharset,
  u2BytesGRCharset,
  u2BytesGRPrefix8EA2Charset,
  u2BytesGRPrefix8EA3Charset,
  u2BytesGRPrefix8EA4Charset,
  u2BytesGRPrefix8EA5Charset,
  u2BytesGRPrefix8EA6Charset,
  u2BytesGRPrefix8EA7Charset
};

static const PRUint16 *g_EUCTWMappingTableSet [] ={
  g_ASCIIMappingTable,
  g_ufCNS1MappingTable,
  g_ufCNS2MappingTable,
  g_ufCNS3MappingTable,
  g_ufCNS4MappingTable,
  g_ufCNS5MappingTable,
  g_ufCNS6MappingTable,
  g_ufCNS7MappingTable
};




NS_METHOD
nsUnicodeToEUCTWConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return CreateMultiTableEncoder(8,
                                 (uScanClassID*) &g_EUCTWScanClassSet,
                                 (uMappingTable**) &g_EUCTWMappingTableSet,
                                 4 ,
                                 aOuter, aIID, aResult);
}

