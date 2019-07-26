














#ifndef USCRIPT_H
#define USCRIPT_H
#include "unicode/utypes.h"





 





















typedef enum UScriptCode {
    






      
      USCRIPT_INVALID_CODE = -1,
      
      USCRIPT_COMMON       =  0,  
      
      USCRIPT_INHERITED    =  1,   
      
      USCRIPT_ARABIC       =  2,  
      
      USCRIPT_ARMENIAN     =  3,  
      
      USCRIPT_BENGALI      =  4,  
      
      USCRIPT_BOPOMOFO     =  5,  
      
      USCRIPT_CHEROKEE     =  6,  
      
      USCRIPT_COPTIC       =  7,  
      
      USCRIPT_CYRILLIC     =  8,  
      
      USCRIPT_DESERET      =  9,  
      
      USCRIPT_DEVANAGARI   = 10,  
      
      USCRIPT_ETHIOPIC     = 11,  
      
      USCRIPT_GEORGIAN     = 12,  
      
      USCRIPT_GOTHIC       = 13,  
      
      USCRIPT_GREEK        = 14,  
      
      USCRIPT_GUJARATI     = 15,  
      
      USCRIPT_GURMUKHI     = 16,  
      
      USCRIPT_HAN          = 17,  
      
      USCRIPT_HANGUL       = 18,  
      
      USCRIPT_HEBREW       = 19,  
      
      USCRIPT_HIRAGANA     = 20,  
      
      USCRIPT_KANNADA      = 21,  
      
      USCRIPT_KATAKANA     = 22,  
      
      USCRIPT_KHMER        = 23,  
      
      USCRIPT_LAO          = 24,  
      
      USCRIPT_LATIN        = 25,  
      
      USCRIPT_MALAYALAM    = 26,  
      
      USCRIPT_MONGOLIAN    = 27,  
      
      USCRIPT_MYANMAR      = 28,  
      
      USCRIPT_OGHAM        = 29,  
      
      USCRIPT_OLD_ITALIC   = 30,  
      
      USCRIPT_ORIYA        = 31,  
      
      USCRIPT_RUNIC        = 32,  
      
      USCRIPT_SINHALA      = 33,  
      
      USCRIPT_SYRIAC       = 34,  
      
      USCRIPT_TAMIL        = 35,  
      
      USCRIPT_TELUGU       = 36,  
      
      USCRIPT_THAANA       = 37,  
      
      USCRIPT_THAI         = 38,  
      
      USCRIPT_TIBETAN      = 39,  
      
      USCRIPT_CANADIAN_ABORIGINAL = 40,  
      
      USCRIPT_UCAS         = USCRIPT_CANADIAN_ABORIGINAL,
      
      USCRIPT_YI           = 41,  
      
      
      USCRIPT_TAGALOG      = 42,  
      
      USCRIPT_HANUNOO      = 43,  
      
      USCRIPT_BUHID        = 44,  
      
      USCRIPT_TAGBANWA     = 45,  

      
      
      USCRIPT_BRAILLE      = 46,  
      
      USCRIPT_CYPRIOT      = 47,  
      
      USCRIPT_LIMBU        = 48,  
      
      USCRIPT_LINEAR_B     = 49,  
      
      USCRIPT_OSMANYA      = 50,  
      
      USCRIPT_SHAVIAN      = 51,  
      
      USCRIPT_TAI_LE       = 52,  
      
      USCRIPT_UGARITIC     = 53,  

      
      USCRIPT_KATAKANA_OR_HIRAGANA = 54,

      
      
      USCRIPT_BUGINESE      = 55, 
      
      USCRIPT_GLAGOLITIC    = 56, 
      
      USCRIPT_KHAROSHTHI    = 57, 
      
      USCRIPT_SYLOTI_NAGRI  = 58, 
      
      USCRIPT_NEW_TAI_LUE   = 59, 
      
      USCRIPT_TIFINAGH      = 60, 
      
      USCRIPT_OLD_PERSIAN   = 61, 

      
      
      USCRIPT_BALINESE                      = 62, 
      
      USCRIPT_BATAK                         = 63, 
      
      USCRIPT_BLISSYMBOLS                   = 64, 
      
      USCRIPT_BRAHMI                        = 65, 
      
      USCRIPT_CHAM                          = 66, 
      
      USCRIPT_CIRTH                         = 67, 
      
      USCRIPT_OLD_CHURCH_SLAVONIC_CYRILLIC  = 68, 
      
      USCRIPT_DEMOTIC_EGYPTIAN              = 69, 
      
      USCRIPT_HIERATIC_EGYPTIAN             = 70, 
      
      USCRIPT_EGYPTIAN_HIEROGLYPHS          = 71, 
      
      USCRIPT_KHUTSURI                      = 72, 
      
      USCRIPT_SIMPLIFIED_HAN                = 73, 
      
      USCRIPT_TRADITIONAL_HAN               = 74, 
      
      USCRIPT_PAHAWH_HMONG                  = 75, 
      
      USCRIPT_OLD_HUNGARIAN                 = 76, 
      
      USCRIPT_HARAPPAN_INDUS                = 77, 
      
      USCRIPT_JAVANESE                      = 78, 
      
      USCRIPT_KAYAH_LI                      = 79, 
      
      USCRIPT_LATIN_FRAKTUR                 = 80, 
      
      USCRIPT_LATIN_GAELIC                  = 81, 
      
      USCRIPT_LEPCHA                        = 82, 
      
      USCRIPT_LINEAR_A                      = 83, 
      
      USCRIPT_MANDAIC                       = 84, 
      
      USCRIPT_MANDAEAN                      = USCRIPT_MANDAIC,
      
      USCRIPT_MAYAN_HIEROGLYPHS             = 85, 
      
      USCRIPT_MEROITIC_HIEROGLYPHS          = 86, 
      
      USCRIPT_MEROITIC                      = USCRIPT_MEROITIC_HIEROGLYPHS,
      
      USCRIPT_NKO                           = 87, 
      
      USCRIPT_ORKHON                        = 88, 
      
      USCRIPT_OLD_PERMIC                    = 89, 
      
      USCRIPT_PHAGS_PA                      = 90, 
      
      USCRIPT_PHOENICIAN                    = 91, 
      
      USCRIPT_PHONETIC_POLLARD              = 92, 
      
      USCRIPT_RONGORONGO                    = 93, 
      
      USCRIPT_SARATI                        = 94, 
      
      USCRIPT_ESTRANGELO_SYRIAC             = 95, 
      
      USCRIPT_WESTERN_SYRIAC                = 96, 
      
      USCRIPT_EASTERN_SYRIAC                = 97, 
      
      USCRIPT_TENGWAR                       = 98, 
      
      USCRIPT_VAI                           = 99, 
      
      USCRIPT_VISIBLE_SPEECH                = 100,
      
      USCRIPT_CUNEIFORM                     = 101,
      
      USCRIPT_UNWRITTEN_LANGUAGES           = 102,
      
      USCRIPT_UNKNOWN                       = 103, 

      
      
      USCRIPT_CARIAN                        = 104,
      
      USCRIPT_JAPANESE                      = 105,
      
      USCRIPT_LANNA                         = 106,
      
      USCRIPT_LYCIAN                        = 107,
      
      USCRIPT_LYDIAN                        = 108,
      
      USCRIPT_OL_CHIKI                      = 109,
      
      USCRIPT_REJANG                        = 110,
      
      USCRIPT_SAURASHTRA                    = 111,
      
      USCRIPT_SIGN_WRITING                  = 112,
      
      USCRIPT_SUNDANESE                     = 113,
      
      USCRIPT_MOON                          = 114,
      
      USCRIPT_MEITEI_MAYEK                  = 115,

      
      
      USCRIPT_IMPERIAL_ARAMAIC              = 116,
      
      USCRIPT_AVESTAN                       = 117,
      
      USCRIPT_CHAKMA                        = 118,
      
      USCRIPT_KOREAN                        = 119,
      
      USCRIPT_KAITHI                        = 120,
      
      USCRIPT_MANICHAEAN                    = 121,
      
      USCRIPT_INSCRIPTIONAL_PAHLAVI         = 122,
      
      USCRIPT_PSALTER_PAHLAVI               = 123,
      
      USCRIPT_BOOK_PAHLAVI                  = 124,
      
      USCRIPT_INSCRIPTIONAL_PARTHIAN        = 125,
      
      USCRIPT_SAMARITAN                     = 126,
      
      USCRIPT_TAI_VIET                      = 127,
      
      USCRIPT_MATHEMATICAL_NOTATION         = 128,
      
      USCRIPT_SYMBOLS                       = 129,

      
      
      USCRIPT_BAMUM                         = 130,
      
      USCRIPT_LISU                          = 131,
      
      USCRIPT_NAKHI_GEBA                    = 132,
      
      USCRIPT_OLD_SOUTH_ARABIAN             = 133,

      
      
      USCRIPT_BASSA_VAH                     = 134,
      
      USCRIPT_DUPLOYAN_SHORTAND             = 135,
      
      USCRIPT_ELBASAN                       = 136,
      
      USCRIPT_GRANTHA                       = 137,
      
      USCRIPT_KPELLE                        = 138,
      
      USCRIPT_LOMA                          = 139,
      
      USCRIPT_MENDE                         = 140,
      
      USCRIPT_MEROITIC_CURSIVE              = 141,
      
      USCRIPT_OLD_NORTH_ARABIAN             = 142,
      
      USCRIPT_NABATAEAN                     = 143,
      
      USCRIPT_PALMYRENE                     = 144,
      
      USCRIPT_SINDHI                        = 145,
      
      USCRIPT_WARANG_CITI                   = 146,

      
      USCRIPT_AFAKA                         = 147,
      
      USCRIPT_JURCHEN                       = 148,
      
      USCRIPT_MRO                           = 149,
      
      USCRIPT_NUSHU                         = 150,
      
      USCRIPT_SHARADA                       = 151,
      
      USCRIPT_SORA_SOMPENG                  = 152,
      
      USCRIPT_TAKRI                         = 153,
      
      USCRIPT_TANGUT                        = 154,
      
      USCRIPT_WOLEAI                        = 155,

      
      USCRIPT_ANATOLIAN_HIEROGLYPHS         = 156,
      
      USCRIPT_KHOJKI                        = 157,
      
      USCRIPT_TIRHUTA                       = 158,

      

      
      USCRIPT_CODE_LIMIT    = 159
} UScriptCode;



















U_STABLE int32_t  U_EXPORT2 
uscript_getCode(const char* nameOrAbbrOrLocale,UScriptCode* fillIn,int32_t capacity,UErrorCode *err);









U_STABLE const char*  U_EXPORT2 
uscript_getName(UScriptCode scriptCode);









U_STABLE const char*  U_EXPORT2 
uscript_getShortName(UScriptCode scriptCode);









U_STABLE UScriptCode  U_EXPORT2 
uscript_getScript(UChar32 codepoint, UErrorCode *err);

#ifndef U_HIDE_DRAFT_API















U_DRAFT UBool U_EXPORT2
uscript_hasScript(UChar32 c, UScriptCode sc);
































U_DRAFT int32_t U_EXPORT2
uscript_getScriptExtensions(UChar32 c,
                            UScriptCode *scripts, int32_t capacity,
                            UErrorCode *errorCode);
#endif  

#endif
