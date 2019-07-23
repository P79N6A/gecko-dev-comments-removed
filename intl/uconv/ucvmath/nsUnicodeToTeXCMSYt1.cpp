





































#include "nsUnicodeToTeXCMSYt1.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmsy-t1.uf"
};




nsUnicodeToTeXCMSYt1::nsUnicodeToTeXCMSYt1() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
