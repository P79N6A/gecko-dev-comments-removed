





































#include "nsUnicodeToMathematica5.h"




static const PRUint16 g_ufMappingTable[] = {
#include "mathematica5.uf"
};




nsUnicodeToMathematica5::nsUnicodeToMathematica5() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
