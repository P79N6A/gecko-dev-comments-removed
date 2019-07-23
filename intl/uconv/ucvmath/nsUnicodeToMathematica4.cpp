





































#include "nsUnicodeToMathematica4.h"




static const PRUint16 g_ufMappingTable[] = {
#include "mathematica4.uf"
};




nsUnicodeToMathematica4::nsUnicodeToMathematica4() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
