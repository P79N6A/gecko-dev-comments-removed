





































#include "nsUnicodeToMathematica2.h"




static const PRUint16 g_ufMappingTable[] = {
#include "mathematica2.uf"
};




nsUnicodeToMathematica2::nsUnicodeToMathematica2() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
