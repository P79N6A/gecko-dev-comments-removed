





































#include "nsUnicodeToTeXCMRttf.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmr-ttf.uf"
};




nsUnicodeToTeXCMRttf::nsUnicodeToTeXCMRttf() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
