




#include "nsGBKConvUtil.h"
#include "gbku.h"
#include "nsDebug.h"
#define MAX_GBK_LENGTH 24066 /* (0xfe-0x80)*(0xfe-0x3f) */




static const char16_t gGBKToUnicodeTable[MAX_GBK_LENGTH] = {
#include "cp936map.h"
};
static const uint16_t gUnicodeToGBKTable[0xA000-0x4e00] = {
#include "cp936invmap.h"
};

bool nsGBKConvUtil::UnicodeToGBKChar(
  char16_t aChar, bool aToGL, char* 
  aOutByte1, char* aOutByte2)
{
  bool found=false;
  *aOutByte1 = *aOutByte2 = 0;
  if(UNICHAR_IN_RANGE(0xd800, aChar, 0xdfff))
  {
    
    return false;
  }
  if(UNICHAR_IN_RANGE(0x4e00, aChar, 0x9FFF))
  {
    uint16_t item = gUnicodeToGBKTable[aChar - 0x4e00];
    if(item != 0) 
    {
      *aOutByte1 = item >> 8;
      *aOutByte2 = item & 0x00FF;
      found = true;
    } else {
      return false;
    }
  } else if (aChar == UCS2_NO_MAPPING) {
    return false;
  } else {
    
    for( int32_t i = 0; i < MAX_GBK_LENGTH; i++ )
    {
      if( aChar == gGBKToUnicodeTable[i])
      {
        *aOutByte1 = (i /  0x00BF + 0x0081) ;
        *aOutByte2 = (i %  0x00BF + 0x0040) ;
        found = true;
        break;
      }
    }
  }
  if(! found)
    return false;

  if(aToGL) {
    
    if(UINT8_IN_RANGE(0xA1, *aOutByte1, 0xFE) &&
       UINT8_IN_RANGE(0xA1, *aOutByte2, 0xFE))
    {
      
      *aOutByte1 &= 0x7F;
      *aOutByte2 &= 0x7F;
    } else {
      
      
      *aOutByte1 = 0x00;
      *aOutByte2 = 0x00;
      return false;
    }
  }
  return true;
}
char16_t nsGBKConvUtil::GBKCharToUnicode(char aByte1, char aByte2)
{
  NS_ASSERTION(UINT8_IN_RANGE(0x81,aByte1, 0xFE), "first byte out of range");
  NS_ASSERTION(UINT8_IN_RANGE(0x40,aByte2, 0xFE), "second byte out of range");

  uint8_t i1 = (uint8_t)aByte1;
  uint8_t i2 = (uint8_t)aByte2;
  uint16_t idx = (i1 - 0x0081) * 0x00bf + i2 - 0x0040 ;

  NS_ASSERTION(idx < MAX_GBK_LENGTH, "ARB");
  
  
  if(idx < MAX_GBK_LENGTH)
    return gGBKToUnicodeTable[ idx ];
  else
    return UCS2_NO_MAPPING;
}
