




































#include "nsUCConstructors.h"
#include "nsUCS2BEToUnicode.h"
#include "nsUCvLatinDll.h"
#include <string.h>
#include "prtypes.h"

#define STATE_NORMAL          0
#define STATE_HALF_CODE_POINT 1
#define STATE_FIRST_CALL      2
#define STATE_FOUND_BOM       3


static nsresult
UTF16ConvertToUnicode(PRUint8& aState, PRUint8& aData, const char * aSrc,
                      PRInt32 * aSrcLength, PRUnichar * aDest,
                      PRInt32 * aDestLength)
{
  const char* src = aSrc;
  const char* srcEnd = aSrc + *aSrcLength;
  PRUnichar* dest = aDest;
  PRUnichar* destEnd = aDest + *aDestLength;

  if(STATE_FOUND_BOM == aState) 
  {
    NS_ASSERTION(*aSrcLength >= 2, "Too few bytes in input");
    src+=2;
    aState = STATE_NORMAL;
  } else if(STATE_FIRST_CALL == aState) { 
    NS_ASSERTION(*aSrcLength >= 2, "Too few bytes in input");

    
    
    
    
    if(0xFEFF == *((PRUnichar*)src)) {
      src+=2;
    } else if(0xFFFE == *((PRUnichar*)src)) {
      *aSrcLength=0;
      *aDestLength=0;
      return NS_ERROR_ILLEGAL_INPUT;
    }  
    aState = STATE_NORMAL;
  }

  PRInt32 copybytes;

  if((STATE_HALF_CODE_POINT == aState) && (src < srcEnd))
  {
    if(dest >= destEnd)
      goto error;

    char tmpbuf[2];

    
    
    
    
    PRUnichar * up = (PRUnichar*) &tmpbuf[0];
    tmpbuf[0]= aData;
    tmpbuf[1]= *src++;
    *dest++ = *up;
  }
  
  copybytes = (destEnd-dest)*2;
  
  if(copybytes > (~1 & (srcEnd - src)))
      copybytes = ~1 & (srcEnd - src);
  memcpy(dest,src,copybytes);
  src +=copybytes;
  dest +=(copybytes/2);
  if(srcEnd == src)  { 
     aState = STATE_NORMAL;
  } else if(1 == (srcEnd - src) ) { 
     aState = STATE_HALF_CODE_POINT;
     aData  = *src++;  
  } else  {
     goto error;
  }
  
  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return NS_OK;

error:
  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return  NS_OK_UDEC_MOREOUTPUT;
}

static void
SwapBytes(PRUnichar *aDest, PRInt32 aLen)
{
  for (PRUnichar *p = aDest; aLen > 0; ++p, --aLen)
     *p = ((*p & 0xff) << 8) | ((*p >> 8) & 0xff);
}

NS_IMETHODIMP
nsUTF16ToUnicodeBase::Reset()
{
  mState = STATE_FIRST_CALL;
  mData = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsUTF16ToUnicodeBase::GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
                                   PRInt32 * aDestLength)
{
  
  *aDestLength = (aSrcLength + ((STATE_HALF_CODE_POINT == mState) ? 1 : 0)) / 2;
  return NS_OK;
}


NS_IMETHODIMP
nsUTF16BEToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
                            PRUnichar * aDest, PRInt32 * aDestLength)
{
#ifdef IS_LITTLE_ENDIAN
    
    
    if(STATE_FIRST_CALL == mState) 
    {
      mState = STATE_NORMAL;
      NS_ASSERTION(*aSrcLength >= 2, "Too few bytes in input");
      if(0xFFFE == *((PRUnichar*)aSrc)) {
        
        mState = STATE_FOUND_BOM;
      } else if(0xFEFF == *((PRUnichar*)aSrc)) {
        *aSrcLength=0;
        *aDestLength=0;
        return NS_ERROR_ILLEGAL_INPUT;
      }
    }
#endif

  nsresult rv = UTF16ConvertToUnicode(mState, mData, aSrc, aSrcLength,
                                      aDest, aDestLength);

#ifdef IS_LITTLE_ENDIAN
  SwapBytes(aDest, *aDestLength);
#endif
  return rv;
}

NS_IMETHODIMP
nsUTF16LEToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
                            PRUnichar * aDest, PRInt32 * aDestLength)
{
#ifdef IS_BIG_ENDIAN
    
    
    if(STATE_FIRST_CALL == mState) 
    {
      mState = STATE_NORMAL;
      NS_ASSERTION(*aSrcLength >= 2, "Too few bytes in input");
      if(0xFFFE == *((PRUnichar*)aSrc)) {
        
        mState = STATE_FOUND_BOM;
      } else if(0xFEFF == *((PRUnichar*)aSrc)) {
        *aSrcLength=0;
        *aDestLength=0;
        return NS_ERROR_ILLEGAL_INPUT;
      }
    }
#endif
    
  nsresult rv = UTF16ConvertToUnicode(mState, mData, aSrc, aSrcLength, aDest,
                                      aDestLength);

#ifdef IS_BIG_ENDIAN
  SwapBytes(aDest, *aDestLength);
#endif
  return rv;
}

NS_IMETHODIMP
nsUTF16ToUnicode::Reset()
{
  mEndian = kUnknown;
  mFoundBOM = PR_FALSE;
  return nsUTF16ToUnicodeBase::Reset();
}

NS_IMETHODIMP
nsUTF16ToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
                          PRUnichar * aDest, PRInt32 * aDestLength)
{
    if(STATE_FIRST_CALL == mState) 
    {
      mState = STATE_NORMAL;
      NS_ASSERTION(*aSrcLength >= 2, "Too few bytes in input");

      
      
      if(0xFF == PRUint8(aSrc[0]) && 0xFE == PRUint8(aSrc[1])) {
        mState = STATE_FOUND_BOM;
        mEndian = kLittleEndian;
        mFoundBOM = PR_TRUE;
      }
      else if(0xFE == PRUint8(aSrc[0]) && 0xFF == PRUint8(aSrc[1])) {
        mState = STATE_FOUND_BOM;
        mEndian = kBigEndian;
        mFoundBOM = PR_TRUE;
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
    
    nsresult rv = UTF16ConvertToUnicode(mState, mData, aSrc, aSrcLength, aDest,
                                        aDestLength);

#ifdef IS_BIG_ENDIAN
    if (mEndian == kLittleEndian)
#elif defined(IS_LITTLE_ENDIAN)
    if (mEndian == kBigEndian)
#else
    #error "Unknown endianness"
#endif
      SwapBytes(aDest, *aDestLength);

    
    
    return (rv == NS_OK && !mFoundBOM) ? NS_OK_UDEC_NOBOMFOUND : rv;
}
