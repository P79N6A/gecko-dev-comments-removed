





































#include "nsUnicodeToTeXCMSYttf.h"




static const PRUint16 g_ufMappingTable[] = {
#include "texcmsy-ttf.uf"
};




nsUnicodeToTeXCMSYttf::nsUnicodeToTeXCMSYttf() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
