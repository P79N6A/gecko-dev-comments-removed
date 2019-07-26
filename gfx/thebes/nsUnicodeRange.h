




#include <stdint.h>

class nsIAtom;








const uint8_t   kRangeCyrillic =    0;
const uint8_t   kRangeGreek    =    1;
const uint8_t   kRangeTurkish  =    2;
const uint8_t   kRangeHebrew   =    3;
const uint8_t   kRangeArabic   =    4;
const uint8_t   kRangeBaltic   =    5;
const uint8_t   kRangeThai     =    6;
const uint8_t   kRangeKorean   =    7;
const uint8_t   kRangeJapanese =    8;
const uint8_t   kRangeSChinese =    9;
const uint8_t   kRangeTChinese =   10;
const uint8_t   kRangeDevanagari = 11;
const uint8_t   kRangeTamil    =   12;
const uint8_t   kRangeArmenian =   13;
const uint8_t   kRangeBengali  =   14;
const uint8_t   kRangeCanadian =   15;
const uint8_t   kRangeEthiopic =   16;
const uint8_t   kRangeGeorgian =   17;
const uint8_t   kRangeGujarati =   18;
const uint8_t   kRangeGurmukhi =   19;
const uint8_t   kRangeKhmer    =   20;
const uint8_t   kRangeMalayalam =  21;
const uint8_t   kRangeOriya     =  22;
const uint8_t   kRangeTelugu    =  23;
const uint8_t   kRangeKannada   =  24;
const uint8_t   kRangeSinhala   =  25;
const uint8_t   kRangeTibetan   =  26;

const uint8_t   kRangeSpecificItemNum = 27;



const uint8_t   kRangeSetStart  =  30;   
const uint8_t   kRangeSetLatin  =  30;
const uint8_t   kRangeSetCJK    =  31;
const uint8_t   kRangeSetEnd    =  31;   
                                         
                                         


const uint8_t   kRangeSurrogate            = 32;
const uint8_t   kRangePrivate              = 33;
const uint8_t   kRangeMisc                 = 34;
const uint8_t   kRangeUnassigned           = 35;
const uint8_t   kRangeSyriac               = 36;
const uint8_t   kRangeThaana               = 37;
const uint8_t   kRangeLao                  = 38;
const uint8_t   kRangeMyanmar              = 39;
const uint8_t   kRangeCherokee             = 40;
const uint8_t   kRangeOghamRunic           = 41;
const uint8_t   kRangeMongolian            = 42;
const uint8_t   kRangeMathOperators        = 43;
const uint8_t   kRangeMiscTechnical        = 44;
const uint8_t   kRangeControlOpticalEnclose = 45;
const uint8_t   kRangeBoxBlockGeometrics   = 46;
const uint8_t   kRangeMiscSymbols          = 47;
const uint8_t   kRangeDingbats             = 48;
const uint8_t   kRangeBraillePattern       = 49;
const uint8_t   kRangeYi                   = 50;
const uint8_t   kRangeCombiningDiacriticalMarks = 51;
const uint8_t   kRangeSpecials             = 52;


const uint8_t   kRangeSMP                  = 53;  
const uint8_t   kRangeHigherPlanes         = 54;  

const uint8_t   kRangeTableBase   = 128;    
const uint8_t   kRangeTertiaryTable  = 145; 
                                            
                                            



uint32_t FindCharUnicodeRange(uint32_t ch);
nsIAtom* LangGroupFromUnicodeRange(uint8_t unicodeRange);
