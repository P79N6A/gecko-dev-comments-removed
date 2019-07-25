




































#include "nsAlgorithm.h"
#include "nsUCSupport.h"
#include "nsUTF8ToUnicode.h"
#include "mozilla/SSE.h"

#define UNICODE_BYTE_ORDER_MARK    0xFEFF

static PRUnichar* EmitSurrogatePair(PRUint32 ucs4, PRUnichar* aDest)
{
  NS_ASSERTION(ucs4 > 0xFFFF, "Should be a supplementary character");
  ucs4 -= 0x00010000;
  *aDest++ = 0xD800 | (0x000003FF & (ucs4 >> 10));
  *aDest++ = 0xDC00 | (0x000003FF & ucs4);
  return aDest;
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












#if defined(__arm__) || defined(_M_ARM)



static inline void
Convert_ascii_run (const char *&src,
                   PRUnichar *&dst,
                   PRInt32 len)
{
  const PRUint32 *src32;
  PRUint32 *dst32;

  
  
  if ((((NS_PTR_TO_UINT32(dst) & 3) == 0) && ((NS_PTR_TO_UINT32(src) & 1) == 0)) ||
      (((NS_PTR_TO_UINT32(dst) & 3) == 2) && ((NS_PTR_TO_UINT32(src) & 1) == 1)))
  {
    while (((NS_PTR_TO_UINT32(src) & 3) ||
            (NS_PTR_TO_UINT32(dst) & 3)) &&
           len > 0)
    {
      if (*src & 0x80U)
        return;
      *dst++ = (PRUnichar) *src++;
      len--;
    }
  } else {
    goto finish;
  }

  
  src32 = (const PRUint32*) src;
  dst32 = (PRUint32*) dst;

  while (len > 4) {
    PRUint32 in = *src32++;

    if (in & 0x80808080U) {
      src32--;
      break;
    }

    *dst32++ = ((in & 0x000000ff) >>  0) | ((in & 0x0000ff00) << 8);
    *dst32++ = ((in & 0x00ff0000) >> 16) | ((in & 0xff000000) >> 8);

    len -= 4;
  }

  src = (const char *) src32;
  dst = (PRUnichar *) dst32;

finish:
  while (len-- > 0 && (*src & 0x80U) == 0) {
    *dst++ = (PRUnichar) *src++;
  }
}

#else

#ifdef MOZILLA_MAY_SUPPORT_SSE2
namespace mozilla {
namespace SSE2 {

void Convert_ascii_run(const char *&src, PRUnichar *&dst, PRInt32 len);

}
}
#endif

static inline void
Convert_ascii_run (const char *&src,
                   PRUnichar *&dst,
                   PRInt32 len)
{
#ifdef MOZILLA_MAY_SUPPORT_SSE2
  if (mozilla::supports_sse2()) {
    mozilla::SSE2::Convert_ascii_run(src, dst, len);
    return;
  }
#endif

  while (len-- > 0 && (*src & 0x80U) == 0) {
    *dst++ = (PRUnichar) *src++;
  }
}

#endif

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

  out = aDest;
  if (mState == 0xFF) {
    
    
    if (aDestLen < 2) {
      NS_ERROR("Output buffer insufficient to hold supplementary character");
      mState = 0;
      return NS_ERROR_ILLEGAL_INPUT;
    }
    out = EmitSurrogatePair(mUcs4, out);
    mUcs4 = 0;
    mState = 0;
    mBytes = 1;
    mFirst = PR_FALSE;
  }

  
  PRInt32 mUcs4 = this->mUcs4;
  PRUint8 mState = this->mState;
  PRUint8 mBytes = this->mBytes;
  PRUint8 mFirst = this->mFirst;

  
  
  if (mFirst && aSrcLen && (0 == (0x80 & (*aSrc))))
    mFirst = PR_FALSE;

  for (in = aSrc; ((in < inend) && (out < outend)); ++in) {
    if (0 == mState) {
      
      
      if (0 == (0x80 & (*in))) {
        PRInt32 max_loops = NS_MIN(inend - in, outend - out);
        Convert_ascii_run(in, out, max_loops);
        --in; 
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
        





        res = NS_ERROR_ILLEGAL_INPUT;
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
            res = NS_ERROR_ILLEGAL_INPUT;
            break;
          }
          if (mUcs4 > 0xFFFF) {
            
            if (out + 2 > outend) {
              
              
              mState = 0xFF;
              ++in;
              res = NS_OK_UDEC_MOREOUTPUT;
              break;
            }
            out = EmitSurrogatePair(mUcs4, out);
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
        res = NS_ERROR_ILLEGAL_INPUT;
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

  this->mUcs4 = mUcs4;
  this->mState = mState;
  this->mBytes = mBytes;
  this->mFirst = mFirst;

  return(res);
}
