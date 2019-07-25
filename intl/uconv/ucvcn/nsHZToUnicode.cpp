
































#include "nsUCvCnDll.h"
#include "nsHZToUnicode.h"
#include "gbku.h"







#define HZ_STATE_GB     1
#define HZ_STATE_ASCII  2
#define HZ_STATE_ODD_BYTE_FLAG 0x80
#define HZLEAD1 '~'
#define HZLEAD2 '{'
#define HZLEAD3 '}'
#define HZ_ODD_BYTE_STATE (mHZState & (HZ_STATE_ODD_BYTE_FLAG))
#define HZ_ENCODING_STATE (mHZState & ~(HZ_STATE_ODD_BYTE_FLAG))

nsHZToUnicode::nsHZToUnicode() : nsBufferDecoderSupport(1)
{
  mHZState = HZ_STATE_ASCII;    
  mRunLength = 0;
  mOddByte = 0;
}


NS_IMETHODIMP nsHZToUnicode::ConvertNoBuff(
  const char* aSrc, 
  int32_t * aSrcLength, 
  PRUnichar *aDest, 
  int32_t * aDestLength)
{
  int32_t i=0;
  int32_t iSrcLength = *aSrcLength;
  int32_t iDestlen = 0;
  *aSrcLength=0;
  nsresult res = NS_OK;
  char oddByte = mOddByte;

  for (i=0; i<iSrcLength; i++) {
    if (iDestlen >= (*aDestLength)) {
      res = NS_OK_UDEC_MOREOUTPUT;
      break;
    }

    char srcByte = *aSrc++;
    (*aSrcLength)++;
    
    if (!HZ_ODD_BYTE_STATE) {
      if (srcByte == HZLEAD1 || 
          (HZ_ENCODING_STATE == HZ_STATE_GB && 
           (UINT8_IN_RANGE(0x21, srcByte, 0x7E) ||
            UINT8_IN_RANGE(0x81, srcByte, 0xFE)))) {
        oddByte = srcByte;
        mHZState |= HZ_STATE_ODD_BYTE_FLAG;
      } else {
        *aDest++ = (srcByte & 0x80) ? UCS2_NO_MAPPING :
                                      CAST_CHAR_TO_UNICHAR(srcByte);
        iDestlen++;
      }
    } else {
      if (oddByte & 0x80) {
        
        NS_ASSERTION(HZ_ENCODING_STATE == HZ_STATE_GB,
                     "Invalid lead byte in ASCII mode");                    
        *aDest++ = (UINT8_IN_RANGE(0x81, oddByte, 0xFE) &&
                    UINT8_IN_RANGE(0x40, srcByte, 0xFE)) ?
                     mUtil.GBKCharToUnicode(oddByte, srcByte) : UCS2_NO_MAPPING;
        mRunLength++;
        iDestlen++;
      
      
      } else if (oddByte == HZLEAD1) { 
        switch (srcByte) {
          case HZLEAD2: 
            
            
            mHZState = HZ_STATE_GB;
            mRunLength = 0;
            break;

          case HZLEAD3: 
            
            
            mHZState = HZ_STATE_ASCII;
            if (mRunLength == 0) {
              *aDest++ = UCS2_NO_MAPPING;
              iDestlen++;
            }
            mRunLength = 0;
            break;

          case HZLEAD1: 
            
            *aDest++ = CAST_CHAR_TO_UNICHAR(srcByte);
            iDestlen++;
            mRunLength++;
            break;

          default:
            
            
            
            
            
            
            
            
            if (srcByte > 0x20 || HZ_ENCODING_STATE == HZ_STATE_ASCII) {
              *aDest++ = UCS2_NO_MAPPING;
            }
            aSrc--;
            (*aSrcLength)--;
            iDestlen++;
            break;
        }
      } else if (HZ_ENCODING_STATE == HZ_STATE_GB) {
        *aDest++ = (UINT8_IN_RANGE(0x21, oddByte, 0x7E) &&
                    UINT8_IN_RANGE(0x21, srcByte, 0x7E)) ?
                     mUtil.GBKCharToUnicode(oddByte|0x80, srcByte|0x80) :
                     UCS2_NO_MAPPING;
        mRunLength++;
        iDestlen++;
      } else {
        NS_NOTREACHED("2-byte sequence that we don't know how to handle");
        *aDest++ = UCS2_NO_MAPPING;
        iDestlen++;
      }
      oddByte = 0;
      mHZState &= ~HZ_STATE_ODD_BYTE_FLAG;
    }
  } 
  mOddByte = HZ_ODD_BYTE_STATE ? oddByte : 0;
  *aDestLength = iDestlen;
  return res;
}


