









































#include "nsBidiUtils.h"
#include "bidicattable.h"
#include "nsCharTraits.h"

static nsCharType ebc2ucd[15] = {
  eCharType_OtherNeutral, 
  eCharType_LeftToRight,
  eCharType_RightToLeft,
  eCharType_RightToLeftArabic,
  eCharType_ArabicNumber,
  eCharType_EuropeanNumber,
  eCharType_EuropeanNumberSeparator,
  eCharType_EuropeanNumberTerminator,
  eCharType_CommonNumberSeparator,
  eCharType_OtherNeutral,
  eCharType_DirNonSpacingMark,
  eCharType_BoundaryNeutral,
  eCharType_BlockSeparator,
  eCharType_SegmentSeparator,
  eCharType_WhiteSpaceNeutral
};

static nsCharType cc2ucd[5] = {
  eCharType_LeftToRightEmbedding,
  eCharType_RightToLeftEmbedding,
  eCharType_PopDirectionalFormat,
  eCharType_LeftToRightOverride,
  eCharType_RightToLeftOverride
};

#define ARABIC_TO_HINDI_DIGIT_INCREMENT (START_HINDI_DIGITS - START_ARABIC_DIGITS)
#define PERSIAN_TO_HINDI_DIGIT_INCREMENT (START_HINDI_DIGITS - START_FARSI_DIGITS)
#define ARABIC_TO_PERSIAN_DIGIT_INCREMENT (START_FARSI_DIGITS - START_ARABIC_DIGITS)
#define NUM_TO_ARABIC(c) \
  ((((c)>=START_HINDI_DIGITS) && ((c)<=END_HINDI_DIGITS)) ? \
   ((c) - (PRUint16)ARABIC_TO_HINDI_DIGIT_INCREMENT) : \
   ((((c)>=START_FARSI_DIGITS) && ((c)<=END_FARSI_DIGITS)) ? \
    ((c) - (PRUint16)ARABIC_TO_PERSIAN_DIGIT_INCREMENT) : \
     (c)))
#define NUM_TO_HINDI(c) \
  ((((c)>=START_ARABIC_DIGITS) && ((c)<=END_ARABIC_DIGITS)) ? \
   ((c) + (PRUint16)ARABIC_TO_HINDI_DIGIT_INCREMENT): \
   ((((c)>=START_FARSI_DIGITS) && ((c)<=END_FARSI_DIGITS)) ? \
    ((c) + (PRUint16)PERSIAN_TO_HINDI_DIGIT_INCREMENT) : \
     (c)))
#define NUM_TO_PERSIAN(c) \
  ((((c)>=START_HINDI_DIGITS) && ((c)<=END_HINDI_DIGITS)) ? \
   ((c) - (PRUint16)PERSIAN_TO_HINDI_DIGIT_INCREMENT) : \
   ((((c)>=START_ARABIC_DIGITS) && ((c)<=END_ARABIC_DIGITS)) ? \
    ((c) + (PRUint16)ARABIC_TO_PERSIAN_DIGIT_INCREMENT) : \
     (c)))

PRUnichar HandleNumberInChar(PRUnichar aChar, bool aPrevCharArabic, PRUint32 aNumFlag)
{
  
  
  
  
  

  switch (aNumFlag) {
    case IBMBIDI_NUMERAL_HINDI:
      return NUM_TO_HINDI(aChar);
    case IBMBIDI_NUMERAL_ARABIC:
      return NUM_TO_ARABIC(aChar);
    case IBMBIDI_NUMERAL_PERSIAN:
      return NUM_TO_PERSIAN(aChar);
    case IBMBIDI_NUMERAL_REGULAR:
    case IBMBIDI_NUMERAL_HINDICONTEXT:
    case IBMBIDI_NUMERAL_PERSIANCONTEXT:
      
      
      if (aPrevCharArabic) {
        if (aNumFlag == IBMBIDI_NUMERAL_PERSIANCONTEXT)
          return NUM_TO_PERSIAN(aChar);
        else
          return NUM_TO_HINDI(aChar);
      }
      else
        return NUM_TO_ARABIC(aChar);
    case IBMBIDI_NUMERAL_NOMINAL:
    default:
      return aChar;
  }
}

nsresult HandleNumbers(PRUnichar* aBuffer, PRUint32 aSize, PRUint32 aNumFlag)
{
  PRUint32 i;

  switch (aNumFlag) {
    case IBMBIDI_NUMERAL_HINDI:
    case IBMBIDI_NUMERAL_ARABIC:
    case IBMBIDI_NUMERAL_PERSIAN:
    case IBMBIDI_NUMERAL_REGULAR:
    case IBMBIDI_NUMERAL_HINDICONTEXT:
    case IBMBIDI_NUMERAL_PERSIANCONTEXT:
      for (i=0;i<aSize;i++)
        aBuffer[i] = HandleNumberInChar(aBuffer[i], !!(i>0 ? aBuffer[i-1] : 0), aNumFlag);
      break;
    case IBMBIDI_NUMERAL_NOMINAL:
    default:
      break;
  }
  return NS_OK;
}

#define LRM_CHAR 0x200e
bool IsBidiControl(PRUint32 aChar)
{
  
  
  
  return (eBidiCat_CC == GetBidiCat(aChar) || ((aChar)&0xfffffe)==LRM_CHAR);
}

bool HasRTLChars(const nsAString& aString)
{




  PRInt32 length = aString.Length();
  for (PRInt32 i = 0; i < length; i++) {
    PRUnichar ch = aString.CharAt(i);
    if (ch >= 0xD800 || IS_IN_BMP_RTL_BLOCK(ch)) {
      return true;
    }
  }
  return false;
}

nsCharType GetCharType(PRUint32 aChar)
{
  nsCharType oResult;
  eBidiCategory bCat = GetBidiCat(aChar);
  if (eBidiCat_CC != bCat) {
    NS_ASSERTION((PRUint32) bCat < (sizeof(ebc2ucd)/sizeof(nsCharType)), "size mismatch");
    if((PRUint32) bCat < (sizeof(ebc2ucd)/sizeof(nsCharType)))
      oResult = ebc2ucd[bCat];
    else 
      oResult = ebc2ucd[0]; 
  } else {
    NS_ASSERTION((aChar-0x202a) < (sizeof(cc2ucd)/sizeof(nsCharType)), "size mismatch");
    if((aChar-0x202a) < (sizeof(cc2ucd)/sizeof(nsCharType)))
      oResult = cc2ucd[aChar - 0x202a];
    else 
      oResult = ebc2ucd[0]; 
  }
  return oResult;
}
