




#include "nsUnicodeToEUCJP.h"
#include "nsUCVJADll.h"
#include "nsUCConstructors.h"





static const int16_t g0201ShiftOutTable[] =  {
        2,
        ShiftOutCell(u1ByteChar,         1, 0x00, 0x00, 0x00, 0x7F),
        ShiftOutCell(u1BytePrefix8EChar, 2, 0x00, 0xA1, 0x00, 0xDF)
};

#define SIZE_OF_TABLES 3
static const uScanClassID gScanClassIDs[SIZE_OF_TABLES] = {
  u2BytesGRCharset,
  u2BytesGRCharset,
  uMultibytesCharset
};

static const int16_t *gShiftTables[SIZE_OF_TABLES] =  {
    0,
    0,
    g0201ShiftOutTable
};

static const uint16_t *gMappingTables[SIZE_OF_TABLES] = {
    g_uf0208Mapping,
    g_uf0208extMapping,
    g_uf0201Mapping
};

nsresult
nsUnicodeToEUCJPConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
    return CreateMultiTableEncoder(SIZE_OF_TABLES,
                                   (uScanClassID*) gScanClassIDs,
                                   (uShiftOutTable**) gShiftTables, 
                                   (uMappingTable**) gMappingTables,
                                   3 ,
                                   aOuter, aIID, aResult);
}

