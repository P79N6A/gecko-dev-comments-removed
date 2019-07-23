





































#include "nsUnicodeToMathematica1.h"




static const PRUint16 g_ufMappingTable[] = {
#include "mathematica1.uf"
};




nsUnicodeToMathematica1::nsUnicodeToMathematica1() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
