





































#include "nsUnicodeToTeXCMMIttf.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmmi-ttf.uf"
};




nsUnicodeToTeXCMMIttf::nsUnicodeToTeXCMMIttf() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
