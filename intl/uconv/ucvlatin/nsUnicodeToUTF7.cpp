




































#include "nsUnicodeToUTF7.h"
#include <string.h>




#define ENC_DIRECT      0
#define ENC_BASE64      1




nsBasicUTF7Encoder::nsBasicUTF7Encoder(char aLastChar, char aEscChar) 
: nsEncoderSupport(5)
{
  mLastChar = aLastChar;
  mEscChar = aEscChar;
  Reset();
}

NS_IMETHODIMP nsBasicUTF7Encoder::FillInfo(PRUint32 *aInfo)
{
  memset(aInfo, 0xFF, (0x10000L >> 3));
  return NS_OK;
}

nsresult nsBasicUTF7Encoder::ShiftEncoding(PRInt32 aEncoding,
                                          char * aDest, 
                                          PRInt32 * aDestLength)
{
  if (aEncoding == mEncoding) {
    *aDestLength = 0;
    return NS_OK;
  } 

  nsresult res = NS_OK;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;

  if (mEncStep != 0) {
    if (dest >= destEnd) return NS_OK_UENC_MOREOUTPUT;
    *(dest++)=ValueToChar(mEncBits);
    mEncStep = 0;
    mEncBits = 0;
  }

  if (dest >= destEnd) {
    res = NS_OK_UENC_MOREOUTPUT;
  } else {
    switch (aEncoding) {
      case 0:
        *(dest++) = '-';
        mEncStep = 0;
        mEncBits = 0;
        break;
      case 1:
        *(dest++) = mEscChar;
        break;
    }
    mEncoding = aEncoding;
  }

  *aDestLength  = dest - aDest;
  return res;
}

nsresult nsBasicUTF7Encoder::EncodeDirect(
                            const PRUnichar * aSrc, 
                            PRInt32 * aSrcLength, 
                            char * aDest, 
                            PRInt32 * aDestLength)
{
  nsresult res = NS_OK;
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;
  PRUnichar ch;

  while (src < srcEnd) {
    ch = *src;

    
    if (!DirectEncodable(ch)) break;

    if (ch == mEscChar) {
      
      if (destEnd - dest < 1) {
        res = NS_OK_UENC_MOREOUTPUT;
        break;
      } else {
        *dest++ = (char)ch;
        *dest++ = (char)'-';
        src++;
      }
    } else {
      
      if (dest >= destEnd) {
        res = NS_OK_UENC_MOREOUTPUT;
        break;
      } else {
        *dest++ = (char)ch;
        src++;
      }
    }
  }

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

nsresult nsBasicUTF7Encoder::EncodeBase64(
                             const PRUnichar * aSrc, 
                             PRInt32 * aSrcLength, 
                             char * aDest, 
                             PRInt32 * aDestLength)
{
  nsresult res = NS_OK;
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;
  PRUnichar ch;
  PRUint32 value;

  while (src < srcEnd) {
    ch = *src;

    
    if (DirectEncodable(ch)) break;

    switch (mEncStep) {
      case 0:
        if (destEnd - dest < 2) {
          res = NS_OK_UENC_MOREOUTPUT;
          break;
        }
        value=ch>>10;
        *(dest++)=ValueToChar(value);
        value=(ch>>4)&0x3f;
        *(dest++)=ValueToChar(value);
        mEncBits=(ch&0x0f)<<2;
        break;
      case 1:
        if (destEnd - dest < 3) {
          res = NS_OK_UENC_MOREOUTPUT;
          break;
        }
        value=mEncBits+(ch>>14);
        *(dest++)=ValueToChar(value);
        value=(ch>>8)&0x3f;
        *(dest++)=ValueToChar(value);
        value=(ch>>2)&0x3f;
        *(dest++)=ValueToChar(value);
        mEncBits=(ch&0x03)<<4;
        break;
      case 2:
        if (destEnd - dest < 3) {
          res = NS_OK_UENC_MOREOUTPUT;
          break;
        }
        value=mEncBits+(ch>>12);
        *(dest++)=ValueToChar(value);
        value=(ch>>6)&0x3f;
        *(dest++)=ValueToChar(value);
        value=ch&0x3f;
        *(dest++)=ValueToChar(value);
        mEncBits=0;
        break;
    }

    if (res != NS_OK) break;

    src++;
    (++mEncStep)%=3;
  }

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

char nsBasicUTF7Encoder::ValueToChar(PRUint32 aValue) { 
  if (aValue < 26) 
    return (char)('A'+aValue);
  else if (aValue < 26 + 26) 
    return (char)('a' + aValue - 26);
  else if (aValue < 26 + 26 + 10)
    return (char)('0' + aValue - 26 - 26);
  else if (aValue == 26 + 26 + 10)
    return '+';
  else if (aValue == 26 + 26 + 10 + 1)
    return mLastChar;
  else
    return -1;
}

PRBool nsBasicUTF7Encoder::DirectEncodable(PRUnichar aChar) {
  
  if ((aChar >= 0x20) && (aChar <= 0x7e)) return PR_TRUE;
  else return PR_FALSE;
}




NS_IMETHODIMP nsBasicUTF7Encoder::ConvertNoBuffNoErr(
                                  const PRUnichar * aSrc, 
                                  PRInt32 * aSrcLength, 
                                  char * aDest, 
                                  PRInt32 * aDestLength)
{
  nsresult res = NS_OK;
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;
  PRInt32 bcr,bcw;
  PRUnichar ch;
  PRInt32 enc;

  while (src < srcEnd) {
    
    ch = *src;
    if (DirectEncodable(ch)) 
      enc = ENC_DIRECT;
    else
      enc = ENC_BASE64;

    
    bcw = destEnd - dest;
    res = ShiftEncoding(enc, dest, &bcw);
    dest += bcw;
    if (res != NS_OK) break;

    
    bcr = srcEnd - src;
    bcw = destEnd - dest;
    if (enc == ENC_DIRECT) 
      res = EncodeDirect(src, &bcr, dest, &bcw);
    else 
      res = EncodeBase64(src, &bcr, dest, &bcw);
    src += bcr;
    dest += bcw;

    if (res != NS_OK) break;
  }

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

NS_IMETHODIMP nsBasicUTF7Encoder::FinishNoBuff(char * aDest, 
                                               PRInt32 * aDestLength)
{
  return ShiftEncoding(ENC_DIRECT, aDest, aDestLength);
}

NS_IMETHODIMP nsBasicUTF7Encoder::Reset()
{
  mEncoding = ENC_DIRECT;
  mEncBits = 0;
  mEncStep = 0;
  return nsEncoderSupport::Reset();
}




nsUnicodeToUTF7::nsUnicodeToUTF7() 
: nsBasicUTF7Encoder('/', '+')
{
}


PRBool nsUnicodeToUTF7::DirectEncodable(PRUnichar aChar) {
  if ((aChar >= 'A') && (aChar <= 'Z')) return PR_TRUE;
  else if ((aChar >= 'a') && (aChar <= 'z')) return PR_TRUE;
  else if ((aChar >= '0') && (aChar <= '9')) return PR_TRUE;
  else if ((aChar >= 39) && (aChar <= 41)) return PR_TRUE;
  else if ((aChar >= 44) && (aChar <= 47)) return PR_TRUE;
  else if (aChar == 58) return PR_TRUE;
  else if (aChar == 63) return PR_TRUE;
  else if (aChar == ' ') return PR_TRUE;
  else if (aChar == 9) return PR_TRUE;
  else if (aChar == 13) return PR_TRUE;
  else if (aChar == 10) return PR_TRUE;
  else if (aChar == 60) return PR_TRUE;  
  else if (aChar == 33) return PR_TRUE;  
  else if (aChar == 34) return PR_TRUE;  
  else if (aChar == 62) return PR_TRUE;  
  else if (aChar == 61) return PR_TRUE;  
  else if (aChar == 59) return PR_TRUE;  
  else if (aChar == 91) return PR_TRUE;  
  else if (aChar == 93) return PR_TRUE;  
  else return PR_FALSE;
}
