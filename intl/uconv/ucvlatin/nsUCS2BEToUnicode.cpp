




































#include "nsUCConstructors.h"
#include "nsUCS2BEToUnicode.h"
#include "nsUCvLatinDll.h"
#include "nsCharTraits.h"
#include <string.h>
#include "prtypes.h"

#define STATE_NORMAL             0
#define STATE_HALF_CODE_POINT    1
#define STATE_FIRST_CALL         2
#define STATE_FOUND_BOM          3
#define STATE_ODD_SURROGATE_PAIR 4

static nsresult
UTF16ConvertToUnicode(PRUint8& aState, PRUint8& aOddByte,
                      PRUnichar& aOddHighSurrogate, PRUnichar& aOddLowSurrogate,
                      const char * aSrc,
                      PRInt32 * aSrcLength, PRUnichar * aDest,
                      PRInt32 * aDestLength,
                      bool aSwapBytes)
{
  const char* src = aSrc;
  const char* srcEnd = aSrc + *aSrcLength;
  PRUnichar* dest = aDest;
  PRUnichar* destEnd = aDest + *aDestLength;

  switch(aState) {
    case STATE_FOUND_BOM:
      NS_ASSERTION(*aSrcLength > 1, "buffer too short");
      src+=2;
      aState = STATE_NORMAL;
      break;

    case STATE_FIRST_CALL: 
      NS_ASSERTION(*aSrcLength > 1, "buffer too short");
      
      
      
      
      if(0xFEFF == *((PRUnichar*)src)) {
        src+=2;
      } else if(0xFFFE == *((PRUnichar*)src)) {
        *aSrcLength=0;
        *aDestLength=0;
        return NS_ERROR_ILLEGAL_INPUT;
      }  
      aState = STATE_NORMAL;
      break;

    case STATE_ODD_SURROGATE_PAIR:
      if (*aDestLength < 2)
        *dest++ = UCS2_REPLACEMENT_CHAR;
      else {
        *dest++ = aOddHighSurrogate;
        *dest++ = aOddLowSurrogate;
        aOddHighSurrogate = aOddLowSurrogate = 0;
        aState = STATE_NORMAL;
      }
      break;

    case STATE_NORMAL:
    case STATE_HALF_CODE_POINT:
    default:
      break;
  }

  if (src == srcEnd) {
    *aDestLength = dest - aDest;
    return NS_OK;
  }

  PRUnichar oddHighSurrogate = aOddHighSurrogate;

  const char* srcEvenEnd;

  PRUnichar u;
  if (aState == STATE_HALF_CODE_POINT) {
    
    
    aState = STATE_NORMAL;
#ifdef IS_BIG_ENDIAN
    u = (aOddByte << 8) | *src++; 
#else
    u = (*src++ << 8) | aOddByte; 
#endif
    srcEvenEnd = src + ((srcEnd - src) & ~1); 
    goto have_codepoint;
  } else {
    srcEvenEnd = src + ((srcEnd - src) & ~1); 
  }

  while (src != srcEvenEnd) {
    if (dest == destEnd)
      goto error;

#if !defined(__sparc__) && !defined(__arm__)
    u = *(const PRUnichar*)src;
#else
    memcpy(&u, src, 2);
#endif
    src += 2;

have_codepoint:
    if (aSwapBytes)
      u = u << 8 | u >> 8;

    if (!IS_SURROGATE(u)) {
      if (oddHighSurrogate) {
        *dest++ = UCS2_REPLACEMENT_CHAR;
        if (dest == destEnd)
          goto error;
        oddHighSurrogate = 0;
      }
      *dest++ = u;
    } else if (NS_IS_HIGH_SURROGATE(u)) {
      if (oddHighSurrogate) {
        *dest++ = UCS2_REPLACEMENT_CHAR;
        if (dest == destEnd)
          goto error;
      }
      oddHighSurrogate = u;
    }
    else  {
      if (oddHighSurrogate && *aDestLength > 1) {
        if (dest + 1 >= destEnd) {
          aOddLowSurrogate = u;
          aOddHighSurrogate = oddHighSurrogate;
          aState = STATE_ODD_SURROGATE_PAIR;
          goto error;
        }
        *dest++ = oddHighSurrogate;
        *dest++ = u;
      } else {
        *dest++ = UCS2_REPLACEMENT_CHAR;
      }
      oddHighSurrogate = 0;
    }
  }
  if (src != srcEnd) {
    
    aOddByte = *src++;
    aState = STATE_HALF_CODE_POINT;
  }

  aOddHighSurrogate = oddHighSurrogate;

  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return NS_OK;

error:
  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return  NS_OK_UDEC_MOREOUTPUT;
}

NS_IMETHODIMP
nsUTF16ToUnicodeBase::Reset()
{
  mState = STATE_FIRST_CALL;
  mOddByte = 0;
  mOddHighSurrogate = 0;
  mOddLowSurrogate = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsUTF16ToUnicodeBase::GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
                                   PRInt32 * aDestLength)
{
  
  *aDestLength = (aSrcLength + ((STATE_HALF_CODE_POINT == mState) ? 1 : 0)) / 2;
  if (mOddHighSurrogate)
    (*aDestLength)++;
  if (mOddLowSurrogate)
    (*aDestLength)++;
  return NS_OK;
}


NS_IMETHODIMP
nsUTF16BEToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
                            PRUnichar * aDest, PRInt32 * aDestLength)
{
    if(STATE_FIRST_CALL == mState && *aSrcLength < 2)
    {
      nsresult res = (*aSrcLength == 0) ? NS_OK : NS_ERROR_ILLEGAL_INPUT;
      *aSrcLength=0;
      *aDestLength=0;
      return res;
    }
#ifdef IS_LITTLE_ENDIAN
    
    
    if(STATE_FIRST_CALL == mState) 
    {
      mState = STATE_NORMAL;
      if(0xFFFE == *((PRUnichar*)aSrc)) {
        
        mState = STATE_FOUND_BOM;
      } else if(0xFEFF == *((PRUnichar*)aSrc)) {
        *aSrcLength=0;
        *aDestLength=0;
        return NS_ERROR_ILLEGAL_INPUT;
      }
    }
#endif

  nsresult rv = UTF16ConvertToUnicode(mState, mOddByte, mOddHighSurrogate,
                                      mOddLowSurrogate,
                                      aSrc, aSrcLength, aDest, aDestLength,
#ifdef IS_LITTLE_ENDIAN
                                      true
#else
                                      false
#endif
                                      );
  return rv;
}

NS_IMETHODIMP
nsUTF16LEToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
                            PRUnichar * aDest, PRInt32 * aDestLength)
{
    if(STATE_FIRST_CALL == mState && *aSrcLength < 2)
    {
      nsresult res = (*aSrcLength == 0) ? NS_OK : NS_ERROR_ILLEGAL_INPUT;
      *aSrcLength=0;
      *aDestLength=0;
      return res;
    }
#ifdef IS_BIG_ENDIAN
    
    
    if(STATE_FIRST_CALL == mState) 
    {
      mState = STATE_NORMAL;
      if(0xFFFE == *((PRUnichar*)aSrc)) {
        
        mState = STATE_FOUND_BOM;
      } else if(0xFEFF == *((PRUnichar*)aSrc)) {
        *aSrcLength=0;
        *aDestLength=0;
        return NS_ERROR_ILLEGAL_INPUT;
      }
    }
#endif
    
  nsresult rv = UTF16ConvertToUnicode(mState, mOddByte, mOddHighSurrogate,
                                      mOddLowSurrogate,
                                      aSrc, aSrcLength, aDest, aDestLength,
#ifdef IS_BIG_ENDIAN
                                      true
#else
                                      false
#endif
                                      );
  return rv;
}

NS_IMETHODIMP
nsUTF16ToUnicode::Reset()
{
  mEndian = kUnknown;
  mFoundBOM = false;
  return nsUTF16ToUnicodeBase::Reset();
}

NS_IMETHODIMP
nsUTF16ToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
                          PRUnichar * aDest, PRInt32 * aDestLength)
{
    if(STATE_FIRST_CALL == mState && *aSrcLength < 2)
    {
      nsresult res = (*aSrcLength == 0) ? NS_OK : NS_ERROR_ILLEGAL_INPUT;
      *aSrcLength=0;
      *aDestLength=0;
      return res;
    }
    if(STATE_FIRST_CALL == mState) 
    {
      mState = STATE_NORMAL;
      
      
      if(0xFF == PRUint8(aSrc[0]) && 0xFE == PRUint8(aSrc[1])) {
        mState = STATE_FOUND_BOM;
        mEndian = kLittleEndian;
        mFoundBOM = true;
      }
      else if(0xFE == PRUint8(aSrc[0]) && 0xFF == PRUint8(aSrc[1])) {
        mState = STATE_FOUND_BOM;
        mEndian = kBigEndian;
        mFoundBOM = true;
      }
      
      
      
      else if(!aSrc[0] && aSrc[1]) {  
        mEndian = kBigEndian;
      }
      else if(aSrc[0] && !aSrc[1]) {  
        mEndian = kLittleEndian;
      }
      else { 
             
             
             
        mEndian = kBigEndian;
      }
    }
    
    nsresult rv = UTF16ConvertToUnicode(mState, mOddByte, mOddHighSurrogate,
                                        mOddLowSurrogate,
                                        aSrc, aSrcLength, aDest, aDestLength,
#ifdef IS_BIG_ENDIAN
                                        (mEndian == kLittleEndian)
#elif defined(IS_LITTLE_ENDIAN)
                                        (mEndian == kBigEndian)
#else
    #error "Unknown endianness"
#endif
                                        );

    
    
    return (rv == NS_OK && !mFoundBOM) ? NS_OK_UDEC_NOBOMFOUND : rv;
}
