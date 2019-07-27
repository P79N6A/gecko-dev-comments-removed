




#include "nsUnicodeRange.h"













































































































































#define NUM_OF_SUBTABLES      10
#define SUBTABLE_SIZE         16

static const uint8_t gUnicodeSubrangeTable[NUM_OF_SUBTABLES][SUBTABLE_SIZE] = 
{ 
  { 
    kRangeTableBase+1,  
    kRangeTableBase+2,  
    kRangeTableBase+3,  
    kRangeSetCJK,       
    kRangeSetCJK,       
    kRangeSetCJK,       
    kRangeSetCJK,       
    kRangeSetCJK,       
    kRangeSetCJK,       
    kRangeSetCJK,       
    kRangeTableBase+4,  
    kRangeKorean,       
    kRangeKorean,       
    kRangeTableBase+5,  
    kRangePrivate,      
    kRangeTableBase+6   
  },
  { 
    kRangeSetLatin,          
    kRangeSetLatin,          
    kRangeSetLatin,          
    kRangeGreek,             
    kRangeCyrillic,          
    kRangeTableBase+7,       
    kRangeArabic,            
    kRangeTertiaryTable,     
    kRangeUnassigned,        
    kRangeTertiaryTable,     
    kRangeTertiaryTable,     
    kRangeTertiaryTable,     
    kRangeTertiaryTable,     
    kRangeTertiaryTable,     
    kRangeTertiaryTable,     
    kRangeTibetan            
  },
  { 
    kRangeTertiaryTable,     
    kRangeKorean,            
    kRangeEthiopic,          
    kRangeTertiaryTable,     
    kRangeCanadian,          
    kRangeCanadian,          
    kRangeTertiaryTable,     
    kRangeKhmer,             
    kRangeMongolian,         
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeSetLatin,          
    kRangeGreek              
  },
  { 
    kRangeSetLatin,          
    kRangeSetLatin,          
    kRangeMathOperators,     
    kRangeMiscTechnical,     
    kRangeControlOpticalEnclose, 
    kRangeBoxBlockGeometrics, 
    kRangeMiscSymbols,       
    kRangeDingbats,          
    kRangeBraillePattern,    
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeSetCJK,            
    kRangeSetCJK             
  },
  {  
    kRangeYi,                
    kRangeYi,                
    kRangeYi,                
    kRangeYi,                
    kRangeYi,                
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean             
  },
  {  
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean,            
    kRangeKorean,            
    kRangeSurrogate,         
    kRangeSurrogate,         
    kRangeSurrogate,         
    kRangeSurrogate,         
    kRangeSurrogate,         
    kRangeSurrogate,         
    kRangeSurrogate,         
    kRangeSurrogate          
  },
  { 
    kRangePrivate,           
    kRangePrivate,           
    kRangePrivate,           
    kRangePrivate,           
    kRangePrivate,           
    kRangePrivate,           
    kRangePrivate,           
    kRangePrivate,           
    kRangePrivate,           
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeTableBase+8,       
    kRangeTableBase+9        
  },
  { 
    kRangeCyrillic,          
    kRangeCyrillic,          
    kRangeCyrillic,          
    kRangeArmenian,          
    kRangeArmenian,          
    kRangeArmenian,          
    kRangeArmenian,          
    kRangeArmenian,          
    kRangeArmenian,          
    kRangeHebrew,            
    kRangeHebrew,            
    kRangeHebrew,            
    kRangeHebrew,            
    kRangeHebrew,            
    kRangeHebrew,            
    kRangeHebrew             
  },
  { 
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic,            
    kRangeArabic             
  },
  { 
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSetCJK,            
    kRangeSpecials,          
  },
};






#define TERTIARY_TABLE_SIZE ((0x1700 - 0x0700) / 0x80)

static const uint8_t gUnicodeTertiaryRangeTable[TERTIARY_TABLE_SIZE] =
{ 
    kRangeSyriac,            
    kRangeThaana,            
    kRangeUnassigned,        
    kRangeUnassigned,        
    kRangeDevanagari,        
    kRangeBengali,           
    kRangeGurmukhi,          
    kRangeGujarati,          
    kRangeOriya,             
    kRangeTamil,             
    kRangeTelugu,            
    kRangeKannada,           
    kRangeMalayalam,         
    kRangeSinhala,           
    kRangeThai,              
    kRangeLao,               
    kRangeTibetan,           
    kRangeTibetan,           
    kRangeMyanmar,           
    kRangeGeorgian,          
    kRangeKorean,            
    kRangeKorean,            
    kRangeEthiopic,          
    kRangeEthiopic,          
    kRangeEthiopic,          
    kRangeCherokee,          
    kRangeCanadian,          
    kRangeCanadian,          
    kRangeCanadian,          
    kRangeCanadian,          
    kRangeCanadian,          
    kRangeOghamRunic         
};









uint32_t FindCharUnicodeRange(uint32_t ch)
{
  uint32_t range;
  
  
  if (ch > 0xFFFF) {
    uint32_t p = (ch >> 16);
    if (p == 1) {
        return kRangeSMP;
    } else if (p == 2) {
        return kRangeSetCJK;
    }
    return kRangeHigherPlanes;
  }

  
  
  range = gUnicodeSubrangeTable[0][ch >> 12];
  
  
  if (range < kRangeTableBase)
    
    return range;

  
  range = gUnicodeSubrangeTable[range - kRangeTableBase][(ch & 0x0f00) >> 8];
  if (range < kRangeTableBase)
    return range;
  if (range < kRangeTertiaryTable)
    return gUnicodeSubrangeTable[range - kRangeTableBase][(ch & 0x00f0) >> 4];

  
  return gUnicodeTertiaryRangeTable[(ch - 0x0700) >> 7];
}
