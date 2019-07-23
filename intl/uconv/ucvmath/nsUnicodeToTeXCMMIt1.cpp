





































#include "nsUnicodeToTeXCMMIt1.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmmi-t1.uf"
};




nsUnicodeToTeXCMMIt1::nsUnicodeToTeXCMMIt1() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable,1)
{
}
