




































#include "nsUCSupport.h"
#include "nsUTF8ToUnicode.h"

#if defined(__GNUC__) && defined(__i386__) && defined(__APPLE__)
#define MAC_SSE2
#endif
#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__)
#define WIN_SSE2
#endif

#if defined(MAC_SSE2) || defined(WIN_SSE2)
#include "emmintrin.h"
#endif

#if defined(MAC_SSE2)
#define __sse2_available 1
#endif

#if defined(WIN_SSE2)
extern "C" int __sse2_available;
#endif

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












#if defined(MAC_SSE2) || defined(WIN_SSE2)

static inline void
Convert_ascii_run (const char *&src,
                   PRUnichar *&dst,
                   PRInt32 len)
{
  if (len > 15 && __sse2_available) {
    __m128i in, out1, out2;
    __m128d *outp1, *outp2;
    __m128i zeroes;
    PRUint32 offset;

    
    while ((NS_PTR_TO_UINT32(src) & 15) && len > 0) {
      if (*src & 0x80U)
        return;
      *dst++ = (PRUnichar) *src++;
      len--;
    }

    zeroes = _mm_setzero_si128();

    offset = NS_PTR_TO_UINT32(dst) & 15;

    
    
    
    

    if (offset == 0) {
      while (len > 15) {
        in = _mm_load_si128((__m128i *) src); 
        if (_mm_movemask_epi8(in))
          break;
        out1 = _mm_unpacklo_epi8(in, zeroes);
        out2 = _mm_unpackhi_epi8(in, zeroes);
        _mm_stream_si128((__m128i *) dst, out1);
        _mm_stream_si128((__m128i *) (dst + 8), out2);
        dst += 16;
        src += 16;
        len -= 16;
      }
    } else if (offset == 8) {
      outp1 = (__m128d *) &out1;
      outp2 = (__m128d *) &out2;
      while (len > 15) {
        in = _mm_load_si128((__m128i *) src); 
        if (_mm_movemask_epi8(in))
          break;
        out1 = _mm_unpacklo_epi8(in, zeroes);
        out2 = _mm_unpackhi_epi8(in, zeroes);
        _mm_storel_epi64((__m128i *) dst, out1);
        _mm_storel_epi64((__m128i *) (dst + 8), out2);
        _mm_storeh_pd((double *) (dst + 4), *outp1);
        _mm_storeh_pd((double *) (dst + 12), *outp2);
        src += 16;
        dst += 16;
        len -= 16;
      }
    } else {
      while (len > 15) {
        in = _mm_load_si128((__m128i *) src);
        if (_mm_movemask_epi8(in))
          break;
        out1 = _mm_unpacklo_epi8(in, zeroes);
        out2 = _mm_unpackhi_epi8(in, zeroes);
        _mm_storeu_si128((__m128i *) dst, out1);
        _mm_storeu_si128((__m128i *) (dst + 8), out2);
        src += 16;
        dst += 16;
        len -= 16;
      }
    }
  }

  

  while (len-- > 0 && (*src & 0x80U) == 0) {
    *dst++ = (PRUnichar) *src++;
  }
}

#elif defined(__arm__) || defined(_M_ARM)



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

static inline void
Convert_ascii_run (const char *&src,
                   PRUnichar *&dst,
                   PRInt32 len)
{
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

  
  PRInt32 mUcs4 = this->mUcs4;
  PRUint8 mState = this->mState;
  PRUint8 mBytes = this->mBytes;
  PRUint8 mFirst = this->mFirst;

  
  
  if (mFirst && aSrcLen && (0 == (0x80 & (*aSrc))))
    mFirst = PR_FALSE;

  for (in = aSrc, out = aDest; ((in < inend) && (out < outend)); ++in) {
    if (0 == mState) {
      
      
      if (0 == (0x80 & (*in))) {
        PRInt32 max_loops = PR_MIN(inend - in, outend - out);
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

  this->mUcs4 = mUcs4;
  this->mState = mState;
  this->mBytes = mBytes;
  this->mFirst = mFirst;

  return(res);
}
