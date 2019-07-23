




































#include "nsUnicodeRange.h"
#include "nsIAtom.h"
#include "gfxAtoms.h"




static nsIAtom **gUnicodeRangeToLangGroupAtomTable[] =
{
  &gfxAtoms::x_cyrillic,
  &gfxAtoms::el,
  &gfxAtoms::tr,
  &gfxAtoms::he,
  &gfxAtoms::ar,
  &gfxAtoms::x_baltic,
  &gfxAtoms::th,
  &gfxAtoms::ko,
  &gfxAtoms::ja,
  &gfxAtoms::zh_cn,
  &gfxAtoms::zh_tw,
  &gfxAtoms::x_devanagari,
  &gfxAtoms::x_tamil,
  &gfxAtoms::x_armn,
  &gfxAtoms::x_beng,
  &gfxAtoms::x_cans,
  &gfxAtoms::x_ethi,
  &gfxAtoms::x_geor,
  &gfxAtoms::x_gujr,
  &gfxAtoms::x_guru,
  &gfxAtoms::x_khmr,
  &gfxAtoms::x_mlym,
  &gfxAtoms::x_orya,
  &gfxAtoms::x_telu,
  &gfxAtoms::x_knda,
  &gfxAtoms::x_sinh
};













































































































































#define NUM_OF_SUBTABLES      9
#define SUBTABLE_SIZE         16

static const PRUint8 gUnicodeSubrangeTable[NUM_OF_SUBTABLES][SUBTABLE_SIZE] = 
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
    kRangeTibetan,           
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
    kRangeGreek,             
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
    kRangeSetCJK,            
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
    kRangeKorean,            
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
    kRangeSurrogate,         
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
    kRangeArabic,            
                             
                             
                             
    kRangeTableBase+8,       
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
    kRangeHebrew,            
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

static const PRUint8 gUnicodeTertiaryRangeTable[TERTIARY_TABLE_SIZE] =
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
    kRangeOghamRunic,        
};









PRUint32 FindCharUnicodeRange(PRUnichar ch)
{
  PRUint32 range;

  
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

nsIAtom *LangGroupFromUnicodeRange(PRUint8 unicodeRange)
{
  if (kRangeSpecificItemNum > unicodeRange) {
    nsIAtom **atom = gUnicodeRangeToLangGroupAtomTable[unicodeRange];
    return *atom;
  }
  return nsnull;
}
