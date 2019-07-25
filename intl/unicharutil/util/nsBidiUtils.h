








































#ifndef nsBidiUtils_h__
#define nsBidiUtils_h__

#include "nsStringGlue.h"

   








typedef enum {
  eBidiCat_Undefined,
  eBidiCat_L,          
  eBidiCat_R,          
  eBidiCat_AL,         
  eBidiCat_AN,         
  eBidiCat_EN,         
  eBidiCat_ES,         
  eBidiCat_ET,         
  eBidiCat_CS,         
  eBidiCat_ON,         
  eBidiCat_NSM,        
  eBidiCat_BN,         
  eBidiCat_B,          
  eBidiCat_S,          
  eBidiCat_WS,         
  eBidiCat_CC = 0xf,   
                       
  eBidiCat_LRE = 0x2a, 
  eBidiCat_RLE = 0x2b, 
  eBidiCat_PDF = 0x2c, 
  eBidiCat_LRO = 0x2d, 
  eBidiCat_RLO = 0x2e  
} eBidiCategory;

enum nsCharType   { 
  eCharType_LeftToRight              = 0, 
  eCharType_RightToLeft              = 1, 
  eCharType_EuropeanNumber           = 2,
  eCharType_EuropeanNumberSeparator  = 3,
  eCharType_EuropeanNumberTerminator = 4,
  eCharType_ArabicNumber             = 5,
  eCharType_CommonNumberSeparator    = 6,
  eCharType_BlockSeparator           = 7,
  eCharType_SegmentSeparator         = 8,
  eCharType_WhiteSpaceNeutral        = 9, 
  eCharType_OtherNeutral             = 10, 
  eCharType_LeftToRightEmbedding     = 11,
  eCharType_LeftToRightOverride      = 12,
  eCharType_RightToLeftArabic        = 13,
  eCharType_RightToLeftEmbedding     = 14,
  eCharType_RightToLeftOverride      = 15,
  eCharType_PopDirectionalFormat     = 16,
  eCharType_DirNonSpacingMark        = 17,
  eCharType_BoundaryNeutral          = 18,
  eCharType_CharTypeCount
};




typedef enum nsCharType nsCharType;





#define CHARTYPE_IS_RTL(val) ( ( (val) == eCharType_RightToLeft) || ( (val) == eCharType_RightToLeftArabic) )

#define CHARTYPE_IS_WEAK(val) ( ( (val) == eCharType_EuropeanNumberSeparator)    \
                           || ( (val) == eCharType_EuropeanNumberTerminator) \
                           || ( ( (val) > eCharType_ArabicNumber) && ( (val) != eCharType_RightToLeftArabic) ) )

  










  PRUnichar HandleNumberInChar(PRUnichar aChar, PRBool aPrevCharArabic, PRUint32 aNumFlag);

  









  nsresult HandleNumbers(PRUnichar* aBuffer, PRUint32 aSize, PRUint32  aNumFlag);

  


  nsCharType GetCharType(PRUint32 aChar);

  




  PRBool IsBidiControl(PRUint32 aChar);

  



  PRBool HasRTLChars(const nsAString& aString);












#define IBMBIDI_TEXTDIRECTION_STR       "bidi.direction"
#define IBMBIDI_TEXTTYPE_STR            "bidi.texttype"
#define IBMBIDI_NUMERAL_STR             "bidi.numeral"
#define IBMBIDI_SUPPORTMODE_STR         "bidi.support"

#define IBMBIDI_TEXTDIRECTION       1
#define IBMBIDI_TEXTTYPE            2
#define IBMBIDI_NUMERAL             4
#define IBMBIDI_SUPPORTMODE         5





#define IBMBIDI_TEXTDIRECTION_LTR     1 //  1 = directionLTRBidi *
#define IBMBIDI_TEXTDIRECTION_RTL     2 //  2 = directionRTLBidi




#define IBMBIDI_TEXTTYPE_CHARSET      1 //  1 = charsettexttypeBidi *
#define IBMBIDI_TEXTTYPE_LOGICAL      2 //  2 = logicaltexttypeBidi
#define IBMBIDI_TEXTTYPE_VISUAL       3 //  3 = visualtexttypeBidi




#define IBMBIDI_NUMERAL_NOMINAL       0 //  0 = nominalnumeralBidi *
#define IBMBIDI_NUMERAL_REGULAR       1 //  1 = regularcontextnumeralBidi
#define IBMBIDI_NUMERAL_HINDICONTEXT  2 //  2 = hindicontextnumeralBidi
#define IBMBIDI_NUMERAL_ARABIC        3 //  3 = arabicnumeralBidi
#define IBMBIDI_NUMERAL_HINDI         4 //  4 = hindinumeralBidi
#define IBMBIDI_NUMERAL_PERSIANCONTEXT 5 // 5 = persiancontextnumeralBidi
#define IBMBIDI_NUMERAL_PERSIAN       6 //  6 = persiannumeralBidi




#define IBMBIDI_SUPPORTMODE_MOZILLA     1 //  1 = mozillaBidisupport *
#define IBMBIDI_SUPPORTMODE_OSBIDI      2 //  2 = OsBidisupport
#define IBMBIDI_SUPPORTMODE_DISABLE     3 //  3 = disableBidisupport

#define IBMBIDI_DEFAULT_BIDI_OPTIONS              \
        ((IBMBIDI_TEXTDIRECTION_LTR<<0)         | \
         (IBMBIDI_TEXTTYPE_CHARSET<<4)          | \
         (IBMBIDI_NUMERAL_NOMINAL<<8)          | \
         (IBMBIDI_SUPPORTMODE_MOZILLA<<12))

#define GET_BIDI_OPTION_DIRECTION(bo) (((bo)>>0) & 0x0000000F) /* 4 bits for DIRECTION */
#define GET_BIDI_OPTION_TEXTTYPE(bo) (((bo)>>4) & 0x0000000F) /* 4 bits for TEXTTYPE */
#define GET_BIDI_OPTION_NUMERAL(bo) (((bo)>>8) & 0x0000000F) /* 4 bits for NUMERAL */
#define GET_BIDI_OPTION_SUPPORT(bo) (((bo)>>12) & 0x0000000F) /* 4 bits for SUPPORT */

#define SET_BIDI_OPTION_DIRECTION(bo, dir) {(bo)=((bo) & 0xFFFFFFF0)|(((dir)& 0x0000000F)<<0);}
#define SET_BIDI_OPTION_TEXTTYPE(bo, tt) {(bo)=((bo) & 0xFFFFFF0F)|(((tt)& 0x0000000F)<<4);}
#define SET_BIDI_OPTION_NUMERAL(bo, num) {(bo)=((bo) & 0xFFFFF0FF)|(((num)& 0x0000000F)<<8);}
#define SET_BIDI_OPTION_SUPPORT(bo, sup) {(bo)=((bo) & 0xFFFF0FFF)|(((sup)& 0x0000000F)<<12);}


#define START_HINDI_DIGITS              0x0660
#define END_HINDI_DIGITS                0x0669
#define START_ARABIC_DIGITS             0x0030
#define END_ARABIC_DIGITS               0x0039
#define START_FARSI_DIGITS              0x06f0
#define END_FARSI_DIGITS                0x06f9
#define IS_HINDI_DIGIT(u)   ( ( (u) >= START_HINDI_DIGITS )  && ( (u) <= END_HINDI_DIGITS ) )
#define IS_ARABIC_DIGIT(u)  ( ( (u) >= START_ARABIC_DIGITS ) && ( (u) <= END_ARABIC_DIGITS ) )
#define IS_FARSI_DIGIT(u)  ( ( (u) >= START_FARSI_DIGITS ) && ( (u) <= END_FARSI_DIGITS ) )











#define IS_ARABIC_SEPARATOR(u) ( ( (u) == 0x0600 ) || ( (u) == 0x0601 ) || ( (u) == 0x0602 ) || ( (u) == 0x0603 ) || ( (u) == 0x066A ) || ( (u) == 0x066B ) || ( (u) == 0x066C ) || ( (u) == 0x06DD ) )

#define IS_BIDI_DIACRITIC(u) ( \
  ( (u) >= 0x0591 && (u) <= 0x05A1) || ( (u) >= 0x05A3 && (u) <= 0x05B9) \
    || ( (u) >= 0x05BB && (u) <= 0x05BD) || ( (u) == 0x05BF) || ( (u) == 0x05C1) \
    || ( (u) == 0x05C2) || ( (u) == 0x05C4) \
    || ( (u) >= 0x064B && (u) <= 0x0652) || ( (u) == 0x0670) \
    || ( (u) >= 0x06D7 && (u) <= 0x06E4) || ( (u) == 0x06E7) || ( (u) == 0x06E8) \
    || ( (u) >= 0x06EA && (u) <= 0x06ED) )

#define IS_HEBREW_CHAR(c) (((0x0590 <= (c)) && ((c)<= 0x05FF)) || (((c) >= 0xfb1d) && ((c) <= 0xfb4f)))
#define IS_ARABIC_CHAR(c) ((0x0600 <= (c)) && ((c)<= 0x06FF))
#define IS_ARABIC_ALPHABETIC(c) (IS_ARABIC_CHAR(c) && \
                                !(IS_HINDI_DIGIT(c) || IS_FARSI_DIGIT(c) || IS_ARABIC_SEPARATOR(c)))
#define IS_BIDI_CONTROL_CHAR(c) (((0x202a <= (c)) && ((c)<= 0x202e)) \
                                || ((c) == 0x200e) || ((c) == 0x200f))










#define IS_IN_BMP_RTL_BLOCK(c) ((0x590 <= (c)) && ((c) <= 0x8ff))
#define IS_RTL_PRESENTATION_FORM(c) (((0xfb1d <= (c)) && ((c) <= 0xfdff)) || \
                                     ((0xfe70 <= (c)) && ((c) <= 0xfefc)))
#define IS_IN_SMP_RTL_BLOCK(c) ((0x10800 <= (c)) && ((c) <= 0x10fff))
#define UCS2_CHAR_IS_BIDI(c) ((IS_IN_BMP_RTL_BLOCK(c)) || \
                              (IS_RTL_PRESENTATION_FORM(c)))
#define UTF32_CHAR_IS_BIDI(c)  ((IS_IN_BMP_RTL_BLOCK(c)) || \
                               (IS_RTL_PRESENTATION_FORM(c)) || \
                               (IS_IN_SMP_RTL_BLOCK(c)))
#endif  
