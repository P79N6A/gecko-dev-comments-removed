





































#include "nsUnicodeToTeXCMRt1.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmr-t1.uf"
};




nsUnicodeToTeXCMRt1::nsUnicodeToTeXCMRt1() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
