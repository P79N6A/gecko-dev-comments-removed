




































#include "nscore.h"








const PRUint8   kRangeCyrillic =    0;
const PRUint8   kRangeGreek    =    1;
const PRUint8   kRangeTurkish  =    2;
const PRUint8   kRangeHebrew   =    3;
const PRUint8   kRangeArabic   =    4;
const PRUint8   kRangeBaltic   =    5;
const PRUint8   kRangeThai     =    6;
const PRUint8   kRangeKorean   =    7;
const PRUint8   kRangeJapanese =    8;
const PRUint8   kRangeSChinese =    9;
const PRUint8   kRangeTChinese =   10;
const PRUint8   kRangeDevanagari = 11;
const PRUint8   kRangeTamil    =   12;
const PRUint8   kRangeArmenian =   13;
const PRUint8   kRangeBengali  =   14;
const PRUint8   kRangeCanadian =   15;
const PRUint8   kRangeEthiopic =   16;
const PRUint8   kRangeGeorgian =   17;
const PRUint8   kRangeGujarati =   18;
const PRUint8   kRangeGurmukhi =   19;
const PRUint8   kRangeKhmer    =   20;
const PRUint8   kRangeMalayalam =  21;

const PRUint8   kRangeSpecificItemNum = 22;



const PRUint8   kRangeSetStart  =  30;    
const PRUint8   kRangeSetLatin  =  30;
const PRUint8   kRangeSetCJK    =  31;
const PRUint8   kRangeSetEnd    =  31;   


const PRUint8   kRangeSurrogate            = 32;
const PRUint8   kRangePrivate              = 33;
const PRUint8   kRangeMisc                 = 34;
const PRUint8   kRangeUnassigned           = 35;
const PRUint8   kRangeSyriac               = 36;
const PRUint8   kRangeThaana               = 37;
const PRUint8   kRangeOriya                = 38;
const PRUint8   kRangeTelugu               = 39;
const PRUint8   kRangeKannada              = 40;
const PRUint8   kRangeSinhala              = 41;
const PRUint8   kRangeLao                  = 42;
const PRUint8   kRangeTibetan              = 43;
const PRUint8   kRangeMyanmar              = 44;
const PRUint8   kRangeCherokee             = 45;
const PRUint8   kRangeOghamRunic           = 46;
const PRUint8   kRangeMongolian            = 47;
const PRUint8   kRangeMathOperators        = 48;
const PRUint8   kRangeMiscTechnical        = 49;
const PRUint8   kRangeControlOpticalEnclose = 50;
const PRUint8   kRangeBoxBlockGeometrics   = 51;
const PRUint8   kRangeMiscSymbols          = 52;
const PRUint8   kRangeDingbats             = 53;
const PRUint8   kRangeBraillePattern       = 54;
const PRUint8   kRangeYi                   = 55;
const PRUint8   kRangeCombiningDiacriticalMarks = 56;
const PRUint8   kRangeSpecials             = 57;

const PRUint8   kRangeTableBase   = 128;    
const PRUint8   kRangeTertiaryTable  = 145; 
                                            
                                            



extern PRUint32 FindCharUnicodeRange(PRUnichar ch);
extern const char* gUnicodeRangeToLangGroupTable[];

inline const char* LangGroupFromUnicodeRange(PRUint8 unicodeRange)
{
  if (kRangeSpecificItemNum > unicodeRange)  
    return gUnicodeRangeToLangGroupTable[unicodeRange];
  return nsnull;
}
