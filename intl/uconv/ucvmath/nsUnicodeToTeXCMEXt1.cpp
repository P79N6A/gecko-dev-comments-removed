





































#include "nsUnicodeToTeXCMEXt1.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmex-t1.uf"
};




nsUnicodeToTeXCMEXt1::nsUnicodeToTeXCMEXt1() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
