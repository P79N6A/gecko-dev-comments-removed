




































#include "nsUCConstructors.h"
#include "nsT61ToUnicode.h"




static const PRUint16 g_T61MappingTable[] = {
#include "t61.ut"
};


static const PRInt16 g_T61ShiftInTable[] =  {
    3,  
    ShiftInCell(u1ByteChar,   1, 0x00, 0xBF),
    ShiftInCell(u1ByteChar,   1, 0xD0, 0xFF),
    ShiftInCell(u2BytesChar,  2, 0xC0, 0xCF)
};

NS_METHOD
nsT61ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                          void **aResult) 
{
  return CreateTableDecoder(uMultibytesCharset,
                            (uShiftInTable*) &g_T61ShiftInTable, 
                            (uMappingTable*) &g_T61MappingTable, 1,
                            aOuter, aIID, aResult);
}

