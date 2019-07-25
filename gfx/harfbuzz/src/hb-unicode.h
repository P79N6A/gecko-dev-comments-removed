

























#ifndef HB_UNICODE_H
#define HB_UNICODE_H

#include "hb-common.h"

HB_BEGIN_DECLS



typedef enum
{
  HB_CATEGORY_CONTROL,
  HB_CATEGORY_FORMAT,
  HB_CATEGORY_UNASSIGNED,
  HB_CATEGORY_PRIVATE_USE,
  HB_CATEGORY_SURROGATE,
  HB_CATEGORY_LOWERCASE_LETTER,
  HB_CATEGORY_MODIFIER_LETTER,
  HB_CATEGORY_OTHER_LETTER,
  HB_CATEGORY_TITLECASE_LETTER,
  HB_CATEGORY_UPPERCASE_LETTER,
  HB_CATEGORY_COMBINING_MARK,
  HB_CATEGORY_ENCLOSING_MARK,
  HB_CATEGORY_NON_SPACING_MARK,
  HB_CATEGORY_DECIMAL_NUMBER,
  HB_CATEGORY_LETTER_NUMBER,
  HB_CATEGORY_OTHER_NUMBER,
  HB_CATEGORY_CONNECT_PUNCTUATION,
  HB_CATEGORY_DASH_PUNCTUATION,
  HB_CATEGORY_CLOSE_PUNCTUATION,
  HB_CATEGORY_FINAL_PUNCTUATION,
  HB_CATEGORY_INITIAL_PUNCTUATION,
  HB_CATEGORY_OTHER_PUNCTUATION,
  HB_CATEGORY_OPEN_PUNCTUATION,
  HB_CATEGORY_CURRENCY_SYMBOL,
  HB_CATEGORY_MODIFIER_SYMBOL,
  HB_CATEGORY_MATH_SYMBOL,
  HB_CATEGORY_OTHER_SYMBOL,
  HB_CATEGORY_LINE_SEPARATOR,
  HB_CATEGORY_PARAGRAPH_SEPARATOR,
  HB_CATEGORY_SPACE_SEPARATOR
} hb_category_t;


typedef enum
{                               
  HB_SCRIPT_INVALID_CODE = -1,
  HB_SCRIPT_COMMON       = 0,   
  HB_SCRIPT_INHERITED,          
  HB_SCRIPT_ARABIC,             
  HB_SCRIPT_ARMENIAN,           
  HB_SCRIPT_BENGALI,            
  HB_SCRIPT_BOPOMOFO,           
  HB_SCRIPT_CHEROKEE,           
  HB_SCRIPT_COPTIC,             
  HB_SCRIPT_CYRILLIC,           
  HB_SCRIPT_DESERET,            
  HB_SCRIPT_DEVANAGARI,         
  HB_SCRIPT_ETHIOPIC,           
  HB_SCRIPT_GEORGIAN,           
  HB_SCRIPT_GOTHIC,             
  HB_SCRIPT_GREEK,              
  HB_SCRIPT_GUJARATI,           
  HB_SCRIPT_GURMUKHI,           
  HB_SCRIPT_HAN,                
  HB_SCRIPT_HANGUL,             
  HB_SCRIPT_HEBREW,             
  HB_SCRIPT_HIRAGANA,           
  HB_SCRIPT_KANNADA,            
  HB_SCRIPT_KATAKANA,           
  HB_SCRIPT_KHMER,              
  HB_SCRIPT_LAO,                
  HB_SCRIPT_LATIN,              
  HB_SCRIPT_MALAYALAM,          
  HB_SCRIPT_MONGOLIAN,          
  HB_SCRIPT_MYANMAR,            
  HB_SCRIPT_OGHAM,              
  HB_SCRIPT_OLD_ITALIC,         
  HB_SCRIPT_ORIYA,              
  HB_SCRIPT_RUNIC,              
  HB_SCRIPT_SINHALA,            
  HB_SCRIPT_SYRIAC,             
  HB_SCRIPT_TAMIL,              
  HB_SCRIPT_TELUGU,             
  HB_SCRIPT_THAANA,             
  HB_SCRIPT_THAI,               
  HB_SCRIPT_TIBETAN,            
  HB_SCRIPT_CANADIAN_ABORIGINAL, 
  HB_SCRIPT_YI,                 
  HB_SCRIPT_TAGALOG,            
  HB_SCRIPT_HANUNOO,            
  HB_SCRIPT_BUHID,              
  HB_SCRIPT_TAGBANWA,           

  
  HB_SCRIPT_BRAILLE,            
  HB_SCRIPT_CYPRIOT,            
  HB_SCRIPT_LIMBU,              
  HB_SCRIPT_OSMANYA,            
  HB_SCRIPT_SHAVIAN,            
  HB_SCRIPT_LINEAR_B,           
  HB_SCRIPT_TAI_LE,             
  HB_SCRIPT_UGARITIC,           

  
  HB_SCRIPT_NEW_TAI_LUE,        
  HB_SCRIPT_BUGINESE,           
  HB_SCRIPT_GLAGOLITIC,         
  HB_SCRIPT_TIFINAGH,           
  HB_SCRIPT_SYLOTI_NAGRI,       
  HB_SCRIPT_OLD_PERSIAN,        
  HB_SCRIPT_KHAROSHTHI,         

  
  HB_SCRIPT_UNKNOWN,            
  HB_SCRIPT_BALINESE,           
  HB_SCRIPT_CUNEIFORM,          
  HB_SCRIPT_PHOENICIAN,         
  HB_SCRIPT_PHAGS_PA,           
  HB_SCRIPT_NKO,                

  
  HB_SCRIPT_KAYAH_LI,           
  HB_SCRIPT_LEPCHA,             
  HB_SCRIPT_REJANG,             
  HB_SCRIPT_SUNDANESE,          
  HB_SCRIPT_SAURASHTRA,         
  HB_SCRIPT_CHAM,               
  HB_SCRIPT_OL_CHIKI,           
  HB_SCRIPT_VAI,                
  HB_SCRIPT_CARIAN,             
  HB_SCRIPT_LYCIAN,             
  HB_SCRIPT_LYDIAN,             

  
  HB_SCRIPT_AVESTAN,                
  HB_SCRIPT_BAMUM,                  
  HB_SCRIPT_EGYPTIAN_HIEROGLYPHS,   
  HB_SCRIPT_IMPERIAL_ARAMAIC,       
  HB_SCRIPT_INSCRIPTIONAL_PAHLAVI,  
  HB_SCRIPT_INSCRIPTIONAL_PARTHIAN, 
  HB_SCRIPT_JAVANESE,               
  HB_SCRIPT_KAITHI,                 
  HB_SCRIPT_LISU,                   
  HB_SCRIPT_MEETEI_MAYEK,           
  HB_SCRIPT_OLD_SOUTH_ARABIAN,      
  HB_SCRIPT_OLD_TURKIC,             
  HB_SCRIPT_SAMARITAN,              
  HB_SCRIPT_TAI_THAM,               
  HB_SCRIPT_TAI_VIET,               

  
  HB_SCRIPT_BATAK,                  
  HB_SCRIPT_BRAHMI,                 
  HB_SCRIPT_MANDAIC                 
} hb_script_t;






typedef struct _hb_unicode_funcs_t hb_unicode_funcs_t;

hb_unicode_funcs_t *
hb_unicode_funcs_create (void);

hb_unicode_funcs_t *
hb_unicode_funcs_reference (hb_unicode_funcs_t *ufuncs);

unsigned int
hb_unicode_funcs_get_reference_count (hb_unicode_funcs_t *ufuncs);

void
hb_unicode_funcs_destroy (hb_unicode_funcs_t *ufuncs);

hb_unicode_funcs_t *
hb_unicode_funcs_copy (hb_unicode_funcs_t *ufuncs);

void
hb_unicode_funcs_make_immutable (hb_unicode_funcs_t *ufuncs);

hb_bool_t
hb_unicode_funcs_is_immutable (hb_unicode_funcs_t *ufuncs);








typedef hb_codepoint_t (*hb_unicode_get_mirroring_func_t) (hb_codepoint_t unicode);
typedef hb_category_t (*hb_unicode_get_general_category_func_t) (hb_codepoint_t unicode);
typedef hb_script_t (*hb_unicode_get_script_func_t) (hb_codepoint_t unicode);
typedef unsigned int (*hb_unicode_get_combining_class_func_t) (hb_codepoint_t unicode);
typedef unsigned int (*hb_unicode_get_eastasian_width_func_t) (hb_codepoint_t unicode);




void
hb_unicode_funcs_set_mirroring_func (hb_unicode_funcs_t *ufuncs,
				     hb_unicode_get_mirroring_func_t mirroring_func);

void
hb_unicode_funcs_set_general_category_func (hb_unicode_funcs_t *ufuncs,
					    hb_unicode_get_general_category_func_t general_category_func);

void
hb_unicode_funcs_set_script_func (hb_unicode_funcs_t *ufuncs,
				  hb_unicode_get_script_func_t script_func);

void
hb_unicode_funcs_set_combining_class_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_combining_class_func_t combining_class_func);

void
hb_unicode_funcs_set_eastasian_width_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_eastasian_width_func_t eastasian_width_func);






hb_unicode_get_mirroring_func_t
hb_unicode_funcs_get_mirroring_func (hb_unicode_funcs_t *ufuncs);

hb_unicode_get_general_category_func_t
hb_unicode_funcs_get_general_category_func (hb_unicode_funcs_t *ufuncs);

hb_unicode_get_script_func_t
hb_unicode_funcs_get_script_func (hb_unicode_funcs_t *ufuncs);

hb_unicode_get_combining_class_func_t
hb_unicode_funcs_get_combining_class_func (hb_unicode_funcs_t *ufuncs);

hb_unicode_get_eastasian_width_func_t
hb_unicode_funcs_get_eastasian_width_func (hb_unicode_funcs_t *ufuncs);




hb_codepoint_t
hb_unicode_get_mirroring (hb_unicode_funcs_t *ufuncs,
			  hb_codepoint_t unicode);

hb_category_t
hb_unicode_get_general_category (hb_unicode_funcs_t *ufuncs,
				 hb_codepoint_t unicode);

hb_script_t
hb_unicode_get_script (hb_unicode_funcs_t *ufuncs,
		       hb_codepoint_t unicode);

unsigned int
hb_unicode_get_combining_class (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode);

unsigned int
hb_unicode_get_eastasian_width (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode);


HB_END_DECLS

#endif 
