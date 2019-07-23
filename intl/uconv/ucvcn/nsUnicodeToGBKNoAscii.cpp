



































 









#include "nsUnicodeToGBKNoAscii.h"




NS_IMETHODIMP nsUnicodeToGBKNoAscii::FillInfo(PRUint32 *aInfo)
{
  nsresult rv = nsUnicodeToGBK::FillInfo(aInfo); 
  if(NS_SUCCEEDED(rv))
  {
    
    aInfo[0] = aInfo[1] = aInfo[2] = aInfo[3] = 0;
  }
  return rv;
}

