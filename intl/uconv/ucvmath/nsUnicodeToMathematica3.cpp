





































#include "nsUnicodeToMathematica3.h"




static const PRUint16 g_ufMappingTable[] = {
#include "mathematica3.uf"
};




nsUnicodeToMathematica3::nsUnicodeToMathematica3() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
