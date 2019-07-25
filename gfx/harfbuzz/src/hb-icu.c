


























#include "hb-private.h"

#include "hb-icu.h"

#include "hb-unicode-private.h"

#include <unicode/uversion.h>
#include <unicode/uchar.h>
#include <unicode/uscript.h>

HB_BEGIN_DECLS


static hb_codepoint_t hb_icu_get_mirroring (hb_codepoint_t unicode) { return u_charMirror(unicode); }
static unsigned int hb_icu_get_combining_class (hb_codepoint_t unicode) { return u_getCombiningClass (unicode); }

static unsigned int
hb_icu_get_eastasian_width (hb_codepoint_t unicode)
{
  switch (u_getIntPropertyValue(unicode, UCHAR_EAST_ASIAN_WIDTH))
  {
  case U_EA_WIDE:
  case U_EA_FULLWIDTH:
    return 2;
  case U_EA_NEUTRAL:
  case U_EA_AMBIGUOUS:
  case U_EA_HALFWIDTH:
  case U_EA_NARROW:
    return 1;
  }
  return 1;
}

static hb_category_t
hb_icu_get_general_category (hb_codepoint_t unicode)
{
  switch (u_getIntPropertyValue(unicode, UCHAR_GENERAL_CATEGORY))
  {
  case U_UNASSIGNED:			return HB_CATEGORY_UNASSIGNED;

  case U_UPPERCASE_LETTER:		return HB_CATEGORY_UPPERCASE_LETTER;	
  case U_LOWERCASE_LETTER:		return HB_CATEGORY_LOWERCASE_LETTER;	
  case U_TITLECASE_LETTER:		return HB_CATEGORY_TITLECASE_LETTER;	
  case U_MODIFIER_LETTER:		return HB_CATEGORY_MODIFIER_LETTER;	
  case U_OTHER_LETTER:			return HB_CATEGORY_OTHER_LETTER;	

  case U_NON_SPACING_MARK:		return HB_CATEGORY_NON_SPACING_MARK;	
  case U_ENCLOSING_MARK:		return HB_CATEGORY_ENCLOSING_MARK;	
  case U_COMBINING_SPACING_MARK:	return HB_CATEGORY_COMBINING_MARK;	

  case U_DECIMAL_DIGIT_NUMBER:		return HB_CATEGORY_DECIMAL_NUMBER;	
  case U_LETTER_NUMBER:			return HB_CATEGORY_LETTER_NUMBER;	
  case U_OTHER_NUMBER:			return HB_CATEGORY_OTHER_NUMBER;	

  case U_SPACE_SEPARATOR:		return HB_CATEGORY_SPACE_SEPARATOR;	
  case U_LINE_SEPARATOR:		return HB_CATEGORY_LINE_SEPARATOR;	
  case U_PARAGRAPH_SEPARATOR:		return HB_CATEGORY_PARAGRAPH_SEPARATOR;	

  case U_CONTROL_CHAR:			return HB_CATEGORY_CONTROL;		
  case U_FORMAT_CHAR:			return HB_CATEGORY_FORMAT;		
  case U_PRIVATE_USE_CHAR:		return HB_CATEGORY_PRIVATE_USE;		
  case U_SURROGATE:			return HB_CATEGORY_SURROGATE;		


  case U_DASH_PUNCTUATION:		return HB_CATEGORY_DASH_PUNCTUATION;	
  case U_START_PUNCTUATION:		return HB_CATEGORY_OPEN_PUNCTUATION;	
  case U_END_PUNCTUATION:		return HB_CATEGORY_CLOSE_PUNCTUATION;	
  case U_CONNECTOR_PUNCTUATION:		return HB_CATEGORY_CONNECT_PUNCTUATION;	
  case U_OTHER_PUNCTUATION:		return HB_CATEGORY_OTHER_PUNCTUATION;	

  case U_MATH_SYMBOL:			return HB_CATEGORY_MATH_SYMBOL;		
  case U_CURRENCY_SYMBOL:		return HB_CATEGORY_CURRENCY_SYMBOL;	
  case U_MODIFIER_SYMBOL:		return HB_CATEGORY_MODIFIER_SYMBOL;	
  case U_OTHER_SYMBOL:			return HB_CATEGORY_OTHER_SYMBOL;	

  case U_INITIAL_PUNCTUATION:		return HB_CATEGORY_INITIAL_PUNCTUATION;	
  case U_FINAL_PUNCTUATION:		return HB_CATEGORY_FINAL_PUNCTUATION;	
  }

  return HB_CATEGORY_UNASSIGNED;
}

static hb_script_t
hb_icu_get_script (hb_codepoint_t unicode)
{
  UErrorCode status = U_ZERO_ERROR;
  UScriptCode scriptCode = uscript_getScript(unicode, &status);
  switch ((int) scriptCode)
  {
#define CHECK_ICU_VERSION(major, minor) \
	U_ICU_VERSION_MAJOR_NUM > (major) || (U_ICU_VERSION_MAJOR_NUM == (major) && U_ICU_VERSION_MINOR_NUM >= (minor))
#define MATCH_SCRIPT(C) case USCRIPT_##C: return HB_SCRIPT_##C
#define MATCH_SCRIPT2(C1, C2) case USCRIPT_##C1: return HB_SCRIPT_##C2
  MATCH_SCRIPT (INVALID_CODE);
  MATCH_SCRIPT (COMMON);             
  MATCH_SCRIPT (INHERITED);          
  MATCH_SCRIPT (ARABIC);             
  MATCH_SCRIPT (ARMENIAN);           
  MATCH_SCRIPT (BENGALI);            
  MATCH_SCRIPT (BOPOMOFO);           
  MATCH_SCRIPT (CHEROKEE);           
  MATCH_SCRIPT (COPTIC);             
  MATCH_SCRIPT (CYRILLIC);           
  MATCH_SCRIPT (DESERET);            
  MATCH_SCRIPT (DEVANAGARI);         
  MATCH_SCRIPT (ETHIOPIC);           
  MATCH_SCRIPT (GEORGIAN);           
  MATCH_SCRIPT (GOTHIC);             
  MATCH_SCRIPT (GREEK);              
  MATCH_SCRIPT (GUJARATI);           
  MATCH_SCRIPT (GURMUKHI);           
  MATCH_SCRIPT (HAN);                
  MATCH_SCRIPT (HANGUL);             
  MATCH_SCRIPT (HEBREW);             
  MATCH_SCRIPT (HIRAGANA);           
  MATCH_SCRIPT (KANNADA);            
  MATCH_SCRIPT (KATAKANA);           
  MATCH_SCRIPT (KHMER);              
  MATCH_SCRIPT (LAO);                
  MATCH_SCRIPT (LATIN);              
  MATCH_SCRIPT (MALAYALAM);          
  MATCH_SCRIPT (MONGOLIAN);          
  MATCH_SCRIPT (MYANMAR);            
  MATCH_SCRIPT (OGHAM);              
  MATCH_SCRIPT (OLD_ITALIC);         
  MATCH_SCRIPT (ORIYA);              
  MATCH_SCRIPT (RUNIC);              
  MATCH_SCRIPT (SINHALA);            
  MATCH_SCRIPT (SYRIAC);             
  MATCH_SCRIPT (TAMIL);              
  MATCH_SCRIPT (TELUGU);             
  MATCH_SCRIPT (THAANA);             
  MATCH_SCRIPT (THAI);               
  MATCH_SCRIPT (TIBETAN);            
  MATCH_SCRIPT (CANADIAN_ABORIGINAL);
  MATCH_SCRIPT (YI);                 
  MATCH_SCRIPT (TAGALOG);            
  MATCH_SCRIPT (HANUNOO);            
  MATCH_SCRIPT (BUHID);              
  MATCH_SCRIPT (TAGBANWA);           

  
  MATCH_SCRIPT (BRAILLE);            
  MATCH_SCRIPT (CYPRIOT);            
  MATCH_SCRIPT (LIMBU);              
  MATCH_SCRIPT (OSMANYA);            
  MATCH_SCRIPT (SHAVIAN);            
  MATCH_SCRIPT (LINEAR_B);           
  MATCH_SCRIPT (TAI_LE);             
  MATCH_SCRIPT (UGARITIC);           

  
  MATCH_SCRIPT (NEW_TAI_LUE);        
  MATCH_SCRIPT (BUGINESE);           
  MATCH_SCRIPT (GLAGOLITIC);         
  MATCH_SCRIPT (TIFINAGH);           
  MATCH_SCRIPT (SYLOTI_NAGRI);       
  MATCH_SCRIPT (OLD_PERSIAN);        
  MATCH_SCRIPT (KHAROSHTHI);         

  
  MATCH_SCRIPT (UNKNOWN);            
  MATCH_SCRIPT (BALINESE);           
  MATCH_SCRIPT (CUNEIFORM);          
  MATCH_SCRIPT (PHOENICIAN);         
  MATCH_SCRIPT (PHAGS_PA);           
  MATCH_SCRIPT (NKO);                

  
  MATCH_SCRIPT (KAYAH_LI);           
  MATCH_SCRIPT (LEPCHA);             
  MATCH_SCRIPT (REJANG);             
  MATCH_SCRIPT (SUNDANESE);          
  MATCH_SCRIPT (SAURASHTRA);         
  MATCH_SCRIPT (CHAM);               
  MATCH_SCRIPT (OL_CHIKI);           
  MATCH_SCRIPT (VAI);                
  MATCH_SCRIPT (CARIAN);             
  MATCH_SCRIPT (LYCIAN);             
  MATCH_SCRIPT (LYDIAN);             

  
  MATCH_SCRIPT (AVESTAN);                
#if CHECK_ICU_VERSION (4, 4)
  MATCH_SCRIPT (BAMUM);                  
#endif
  MATCH_SCRIPT (EGYPTIAN_HIEROGLYPHS);   
  MATCH_SCRIPT (IMPERIAL_ARAMAIC);       
  MATCH_SCRIPT (INSCRIPTIONAL_PAHLAVI);  
  MATCH_SCRIPT (INSCRIPTIONAL_PARTHIAN); 
  MATCH_SCRIPT (JAVANESE);               
  MATCH_SCRIPT (KAITHI);                 
  MATCH_SCRIPT2(LANNA, TAI_THAM);        
#if CHECK_ICU_VERSION (4, 4)
  MATCH_SCRIPT (LISU);                   
#endif
  MATCH_SCRIPT2(MEITEI_MAYEK, MEETEI_MAYEK);
#if CHECK_ICU_VERSION (4, 4)
  MATCH_SCRIPT (OLD_SOUTH_ARABIAN);      
#endif
  MATCH_SCRIPT2(ORKHON, OLD_TURKIC);     
  MATCH_SCRIPT (SAMARITAN);              
  MATCH_SCRIPT (TAI_VIET);               

  
  MATCH_SCRIPT (BATAK);                  
  MATCH_SCRIPT (BRAHMI);                 
  MATCH_SCRIPT2(MANDAEAN, MANDAIC);      

  }
  return HB_SCRIPT_UNKNOWN;
}

static hb_unicode_funcs_t icu_ufuncs = {
  HB_REFERENCE_COUNT_INVALID, 
  TRUE, 
  {
    hb_icu_get_general_category,
    hb_icu_get_combining_class,
    hb_icu_get_mirroring,
    hb_icu_get_script,
    hb_icu_get_eastasian_width
  }
};

hb_unicode_funcs_t *
hb_icu_get_unicode_funcs (void)
{
  return &icu_ufuncs;
}


HB_END_DECLS
