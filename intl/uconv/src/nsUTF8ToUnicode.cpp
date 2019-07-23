




































#include "nsUCSupport.h"
#include "nsUTF8ToUnicode.h"

#define UNICODE_BYTE_ORDER_MARK    0xFEFF

NS_IMETHODIMP NS_NewUTF8ToUnicode(nsISupports* aOuter,
                                  const nsIID& aIID,
                                  void** aResult)
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aOuter) {
    *aResult = nsnull;
    return NS_ERROR_NO_AGGREGATION;
  }
  nsUTF8ToUnicode * inst = new nsUTF8ToUnicode();
  if (!inst) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult res = inst->QueryInterface(aIID, aResult);
  if (NS_FAILED(res)) {
    *aResult = nsnull;
    delete inst;
  }
  return res;
}




nsUTF8ToUnicode::nsUTF8ToUnicode()
: nsBasicDecoderSupport()
{
  Reset();
}



















NS_IMETHODIMP nsUTF8ToUnicode::GetMaxLength(const char * aSrc,
                                            PRInt32 aSrcLength,
                                            PRInt32 * aDestLength)
{
  *aDestLength = aSrcLength + 1;
  return NS_OK;
}





NS_IMETHODIMP nsUTF8ToUnicode::Reset()
{

  mUcs4  = 0;     
  mState = 0;     
                  
  mBytes = 1;     
  mFirst = PR_TRUE;

  return NS_OK;

}





NS_IMETHODIMP nsUTF8ToUnicode::Convert(const char * aSrc,
                                       PRInt32 * aSrcLength,
                                       PRUnichar * aDest,
                                       PRInt32 * aDestLength)
{
  PRUint32 aSrcLen   = (PRUint32) (*aSrcLength);
  PRUint32 aDestLen = (PRUint32) (*aDestLength);

  const char *in, *inend;
  inend = aSrc + aSrcLen;

  PRUnichar *out, *outend;
  outend = aDest + aDestLen;

  nsresult res = NS_OK; 

  
  
  if (mFirst && aSrcLen && (0 == (0x80 & (*aSrc))))
    mFirst = PR_FALSE;

  for (in = aSrc, out = aDest; ((in < inend) && (out < outend)); ++in) {
    if (0 == mState) {
      
      
      if (0 == (0x80 & (*in))) {
        
        *out++ = (PRUnichar)*in;
        mBytes = 1;
      } else if (0xC0 == (0xE0 & (*in))) {
        
        mUcs4 = (PRUint32)(*in);
        mUcs4 = (mUcs4 & 0x1F) << 6;
        mState = 1;
        mBytes = 2;
      } else if (0xE0 == (0xF0 & (*in))) {
        
        mUcs4 = (PRUint32)(*in);
        mUcs4 = (mUcs4 & 0x0F) << 12;
        mState = 2;
        mBytes = 3;
      } else if (0xF0 == (0xF8 & (*in))) {
        
        mUcs4 = (PRUint32)(*in);
        mUcs4 = (mUcs4 & 0x07) << 18;
        mState = 3;
        mBytes = 4;
      } else if (0xF8 == (0xFC & (*in))) {
        







        mUcs4 = (PRUint32)(*in);
        mUcs4 = (mUcs4 & 0x03) << 24;
        mState = 4;
        mBytes = 5;
      } else if (0xFC == (0xFE & (*in))) {
        
        mUcs4 = (PRUint32)(*in);
        mUcs4 = (mUcs4 & 1) << 30;
        mState = 5;
        mBytes = 6;
      } else {
        





        res = NS_ERROR_UNEXPECTED;
        break;
      }
    } else {
      
      
      if (0x80 == (0xC0 & (*in))) {
        
        PRUint32 shift = (mState - 1) * 6;
        PRUint32 tmp = *in;
        tmp = (tmp & 0x0000003FL) << shift;
        mUcs4 |= tmp;

        if (0 == --mState) {
          





          
          if (((2 == mBytes) && (mUcs4 < 0x0080)) ||
              ((3 == mBytes) && (mUcs4 < 0x0800)) ||
              ((4 == mBytes) && (mUcs4 < 0x10000)) ||
              (4 < mBytes) ||
              
              ((mUcs4 & 0xFFFFF800) == 0xD800) ||
              
              (mUcs4 > 0x10FFFF)) {
            res = NS_ERROR_UNEXPECTED;
            break;
          }
          if (mUcs4 > 0xFFFF) {
            
            mUcs4 -= 0x00010000;
            *out++ = 0xD800 | (0x000003FF & (mUcs4 >> 10));
            *out++ = 0xDC00 | (0x000003FF & mUcs4);
          } else if (UNICODE_BYTE_ORDER_MARK != mUcs4 || !mFirst) {
            
            *out++ = mUcs4;
          }
          
          mUcs4  = 0;
          mState = 0;
          mBytes = 1;
          mFirst = PR_FALSE;
        }
      } else {
        





        in--;
        res = NS_ERROR_UNEXPECTED;
        break;
      }
    }
  }

  
  if ((NS_OK == res) && (in < inend) && (out >= outend))
    res = NS_OK_UDEC_MOREOUTPUT;

  
  
  if ((NS_OK == res) && (mState != 0))
    res = NS_OK_UDEC_MOREINPUT;

  *aSrcLength = in - aSrc;
  *aDestLength = out - aDest;

  return(res);
}
