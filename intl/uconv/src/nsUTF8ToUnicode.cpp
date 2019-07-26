





#include "nsAlgorithm.h"
#include "nsUCSupport.h"
#include "nsUTF8ToUnicode.h"
#include "mozilla/SSE.h"

#define UNICODE_BYTE_ORDER_MARK    0xFEFF

static PRUnichar* EmitSurrogatePair(uint32_t ucs4, PRUnichar* aDest)
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
                                            int32_t aSrcLength,
                                            int32_t * aDestLength)
{
  *aDestLength = aSrcLength + 1;
  return NS_OK;
}





NS_IMETHODIMP nsUTF8ToUnicode::Reset()
{

  mUcs4  = 0;     
  mState = 0;     
                  
  mBytes = 1;     
  mFirst = true;

  return NS_OK;

}












#if defined(__arm__) || defined(_M_ARM)



static inline void
Convert_ascii_run (const char *&src,
                   PRUnichar *&dst,
                   int32_t len)
{
  const uint32_t *src32;
  uint32_t *dst32;

  
  
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

  
  src32 = (const uint32_t*) src;
  dst32 = (uint32_t*) dst;

  while (len > 4) {
    uint32_t in = *src32++;

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

void Convert_ascii_run(const char *&src, PRUnichar *&dst, int32_t len);

}
}
#endif

static inline void
Convert_ascii_run (const char *&src,
                   PRUnichar *&dst,
                   int32_t len)
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
                                       int32_t * aSrcLength,
                                       PRUnichar * aDest,
                                       int32_t * aDestLength)
{
  uint32_t aSrcLen   = (uint32_t) (*aSrcLength);
  uint32_t aDestLen = (uint32_t) (*aDestLength);

  const char *in, *inend;
  inend = aSrc + aSrcLen;

  PRUnichar *out, *outend;
  outend = aDest + aDestLen;

  nsresult res = NS_OK; 

  out = aDest;
  if (mState == 0xFF) {
    
    
    if (aDestLen < 2) {
      *aSrcLength = *aDestLength = 0;
      return NS_OK_UDEC_MOREOUTPUT;
    }
    out = EmitSurrogatePair(mUcs4, out);
    mUcs4 = 0;
    mState = 0;
    mBytes = 1;
    mFirst = false;
  }

  
  int32_t mUcs4 = this->mUcs4;
  uint8_t mState = this->mState;
  uint8_t mBytes = this->mBytes;
  bool mFirst = this->mFirst;

  
  
  if (mFirst && aSrcLen && (0 == (0x80 & (*aSrc))))
    mFirst = false;

  for (in = aSrc; ((in < inend) && (out < outend)); ++in) {
    uint8_t c = *in;
    if (0 == mState) {
      
      
      if (c < 0x80) {  
        int32_t max_loops = NS_MIN(inend - in, outend - out);
        Convert_ascii_run(in, out, max_loops);
        --in; 
        mBytes = 1;
      } else if (c < 0xC2) {  
        
        if (mErrBehavior == kOnError_Signal) {
          res = NS_ERROR_ILLEGAL_INPUT;
          break;
        }
        *out++ = UCS2_REPLACEMENT_CHAR;
        mFirst = false;
      } else if (c < 0xE0) {  
        
        mUcs4 = c;
        mUcs4 = (mUcs4 & 0x1F) << 6;
        mState = 1;
        mBytes = 2;
      } else if (c < 0xF0) {  
        
        mUcs4 = c;
        mUcs4 = (mUcs4 & 0x0F) << 12;
        mState = 2;
        mBytes = 3;
      } else if (c < 0xF5) {  
        
        mUcs4 = c;
        mUcs4 = (mUcs4 & 0x07) << 18;
        mState = 3;
        mBytes = 4;
      } else {  
        


        if (mErrBehavior == kOnError_Signal) {
          


          res = NS_ERROR_ILLEGAL_INPUT;
          break;
        }
        *out++ = UCS2_REPLACEMENT_CHAR;
        mFirst = false;
      }
    } else {
      
      
      if (0x80 == (0xC0 & c)) {
        if (mState > 1) {
          
          
          
          
          if ((mBytes == 3 && ((!mUcs4 && c < 0xA0) ||             
                               (mUcs4 == 0xD000 && c > 0x9F))) ||  
              (mState == 3 && ((!mUcs4 && c < 0x90) ||             
                               (mUcs4 == 0x100000 && c > 0x8F)))) {
            
            in--;
            if (mErrBehavior == kOnError_Signal) {
              res = NS_ERROR_ILLEGAL_INPUT;
              break;
            }
            *out++ = UCS2_REPLACEMENT_CHAR;
            mState = 0;
            mFirst = false;
            continue;
          }
        }

        
        uint32_t shift = (mState - 1) * 6;
        uint32_t tmp = c;
        tmp = (tmp & 0x0000003FL) << shift;
        mUcs4 |= tmp;

        if (0 == --mState) {
          



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
          mFirst = false;
        }
      } else {
        





        in--;
        if (mErrBehavior == kOnError_Signal) {
          res = NS_ERROR_ILLEGAL_INPUT;
          break;
        }
        *out++ = UCS2_REPLACEMENT_CHAR;
        mState = 0;
        mFirst = false;
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
