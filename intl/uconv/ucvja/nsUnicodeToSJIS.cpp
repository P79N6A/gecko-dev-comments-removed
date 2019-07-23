




































#include "nsUCConstructors.h"
#include "nsUnicodeToSJIS.h"




static const PRUint16 g_SJISMappingTable[] = {
#include "sjis.uf"
};

static const PRInt16 g_SJISShiftOutTable[] =  {
  4,
  ShiftOutCell(u1ByteChar,   1, 0x00, 0x00, 0x00, 0x7F),
  ShiftOutCell(u1ByteChar,   1, 0x00, 0xA1, 0x00, 0xDF),
  ShiftOutCell(u2BytesChar,  2, 0x81, 0x40, 0x9F, 0xFC),
  ShiftOutCell(u2BytesChar,  2, 0xE0, 0x40, 0xFC, 0xFC)
};

NS_METHOD
nsUnicodeToSJISConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return CreateTableEncoder(uMultibytesCharset,
                            (uShiftOutTable*) &g_SJISShiftOutTable, 
                            (uMappingTable*) &g_SJISMappingTable,
                            2 ,
                            aOuter, aIID, aResult);
}

