





































#include "nsUnicodeToMTExtra.h"




static const PRUint16 g_ufMappingTable[] = {
#include "mtextra.uf"
};




nsUnicodeToMTExtra::nsUnicodeToMTExtra() 
  : nsTableEncoderSupport(u1ByteCharset,
                        (uMappingTable*) &g_ufMappingTable, 1)
{
}
