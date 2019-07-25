




































#include "nsGBKConvUtil.h"
#include "gbku.h"
#include "nsCRT.h"
#define MAX_GBK_LENGTH 24066 /* (0xfe-0x80)*(0xfe-0x3f) */




static bool gInitToGBKTable = false;
static const PRUnichar gGBKToUnicodeTable[MAX_GBK_LENGTH] = {
#include "cp936map.h"
};
static PRUint16 gUnicodeToGBKTable[0xA000-0x4e00];

bool nsGBKConvUtil::UnicodeToGBKChar(
  PRUnichar aChar, bool aToGL, char* 
  aOutByte1, char* aOutByte2)
{
  NS_ASSERTION(gInitToGBKTable, "gGBKToUnicodeTable is not init yet. need to call InitToGBKTable first");
  bool found=false;
  *aOutByte1 = *aOutByte2 = 0;
  if(UNICHAR_IN_RANGE(0xd800, aChar, 0xdfff))
  {
    
    return PR_FALSE;
  }
  if(UNICHAR_IN_RANGE(0x4e00, aChar, 0x9FFF))
  {
    PRUint16 item = gUnicodeToGBKTable[aChar - 0x4e00];
    if(item != 0) 
    {
      *aOutByte1 = item >> 8;
      *aOutByte2 = item & 0x00FF;
      found = PR_TRUE;
    } else {
      return PR_FALSE;
    }
  } else {
    
    for( PRInt32 i = 0; i < MAX_GBK_LENGTH; i++ )
    {
      if( aChar == gGBKToUnicodeTable[i])
      {
        *aOutByte1 = (i /  0x00BF + 0x0081) ;
        *aOutByte2 = (i %  0x00BF + 0x0040) ;
        found = PR_TRUE;
        break;
      }
    }
  }
  if(! found)
    return PR_FALSE;

  if(aToGL) {
    
    if(UINT8_IN_RANGE(0xA1, *aOutByte1, 0xFE) &&
       UINT8_IN_RANGE(0xA1, *aOutByte2, 0xFE))
    {
      
      *aOutByte1 &= 0x7F;
      *aOutByte2 &= 0x7F;
    } else {
      
      
      *aOutByte1 = 0x00;
      *aOutByte2 = 0x00;
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}
PRUnichar nsGBKConvUtil::GBKCharToUnicode(char aByte1, char aByte2)
{
  NS_ASSERTION(UINT8_IN_RANGE(0x81,aByte1, 0xFE), "first byte out of range");
  NS_ASSERTION(UINT8_IN_RANGE(0x40,aByte2, 0xFE), "second byte out of range");

  PRUint8 i1 = (PRUint8)aByte1;
  PRUint8 i2 = (PRUint8)aByte2;
  PRUint16 idx = (i1 - 0x0081) * 0x00bf + i2 - 0x0040 ;

  NS_ASSERTION(idx < MAX_GBK_LENGTH, "ARB");
  
  
  if(idx < MAX_GBK_LENGTH)
    return gGBKToUnicodeTable[ idx ];
  else
    return UCS2_NO_MAPPING;
}
void nsGBKConvUtil::InitToGBKTable()
{
  if ( gInitToGBKTable )
   return;

  PRUnichar unicode;
  PRUnichar i;
  
  memset(gUnicodeToGBKTable,0, sizeof(gUnicodeToGBKTable));

  for ( i=0; i<MAX_GBK_LENGTH; i++ )
  {
    unicode = gGBKToUnicodeTable[i];
    
    
    
    if(UNICHAR_IN_RANGE(0x4e00, unicode, 0x9fff))
    {
      unicode -= 0x4E00; 
      gUnicodeToGBKTable[unicode] =  (( i / 0x00BF + 0x0081) << 8) | 
                                    ( i % 0x00BF+ 0x0040);
    }
  }
  gInitToGBKTable = PR_TRUE;
}
