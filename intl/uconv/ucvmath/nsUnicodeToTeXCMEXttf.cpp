





































#include "nsUnicodeToTeXCMEXttf.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmex-ttf.uf"
};




nsUnicodeToTeXCMEXttf::nsUnicodeToTeXCMEXttf() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
