




































#include "nsUnicodeToBIG5NoAscii.h"
#include "nsUCvTWDll.h"




nsUnicodeToBIG5NoAscii::nsUnicodeToBIG5NoAscii() 
  : nsTableEncoderSupport(u2BytesCharset,
                        (uMappingTable*) &g_ufBig5Mapping,
                         2 )
{
}


NS_IMETHODIMP nsUnicodeToBIG5NoAscii::FillInfo(PRUint32 *aInfo)
{
  nsresult rv = nsTableEncoderSupport::FillInfo(aInfo); 
  if(NS_SUCCEEDED(rv))
  {
    
    aInfo[0] = aInfo[1] = aInfo[2] = aInfo[3] = 0;
  }
  return rv;
}
