





















#ifndef UCHAR_H
#define UCHAR_H

#include "unicode/utypes.h"

U_CDECL_BEGIN













#define U_UNICODE_VERSION "6.2"

















































































#define UCHAR_MIN_VALUE 0









#define UCHAR_MAX_VALUE 0x10ffff





#define U_MASK(x) ((uint32_t)1<<(x))




















typedef enum UProperty {
    





    




    

    UCHAR_ALPHABETIC=0,
    
    UCHAR_BINARY_START=UCHAR_ALPHABETIC,
    
    UCHAR_ASCII_HEX_DIGIT=1,
    


    UCHAR_BIDI_CONTROL=2,
    



    UCHAR_BIDI_MIRRORED=3,
    
    UCHAR_DASH=4,
    


    UCHAR_DEFAULT_IGNORABLE_CODE_POINT=5,
    

    UCHAR_DEPRECATED=6,
    

    UCHAR_DIACRITIC=7,
    


    UCHAR_EXTENDER=8,
    


    UCHAR_FULL_COMPOSITION_EXCLUSION=9,
    


    UCHAR_GRAPHEME_BASE=10,
    


    UCHAR_GRAPHEME_EXTEND=11,
    

    UCHAR_GRAPHEME_LINK=12,
    

    UCHAR_HEX_DIGIT=13,
    

    UCHAR_HYPHEN=14,
    



    UCHAR_ID_CONTINUE=15,
    


    UCHAR_ID_START=16,
    

    UCHAR_IDEOGRAPHIC=17,
    


    UCHAR_IDS_BINARY_OPERATOR=18,
    


    UCHAR_IDS_TRINARY_OPERATOR=19,
    

    UCHAR_JOIN_CONTROL=20,
    


    UCHAR_LOGICAL_ORDER_EXCEPTION=21,
    

    UCHAR_LOWERCASE=22,
    
    UCHAR_MATH=23,
    


    UCHAR_NONCHARACTER_CODE_POINT=24,
    
    UCHAR_QUOTATION_MARK=25,
    


    UCHAR_RADICAL=26,
    



    UCHAR_SOFT_DOTTED=27,
    


    UCHAR_TERMINAL_PUNCTUATION=28,
    


    UCHAR_UNIFIED_IDEOGRAPH=29,
    

    UCHAR_UPPERCASE=30,
    


    UCHAR_WHITE_SPACE=31,
    


    UCHAR_XID_CONTINUE=32,
    

    UCHAR_XID_START=33,
    


   UCHAR_CASE_SENSITIVE=34,
    



    UCHAR_S_TERM=35,
    




    UCHAR_VARIATION_SELECTOR=36,
    





    UCHAR_NFD_INERT=37,
    





    UCHAR_NFKD_INERT=38,
    





    UCHAR_NFC_INERT=39,
    





    UCHAR_NFKC_INERT=40,
    









    UCHAR_SEGMENT_STARTER=41,
    



    UCHAR_PATTERN_SYNTAX=42,
    



    UCHAR_PATTERN_WHITE_SPACE=43,
    



    UCHAR_POSIX_ALNUM=44,
    



    UCHAR_POSIX_BLANK=45,
    



    UCHAR_POSIX_GRAPH=46,
    



    UCHAR_POSIX_PRINT=47,
    



    UCHAR_POSIX_XDIGIT=48,
    
    UCHAR_CASED=49,
    
    UCHAR_CASE_IGNORABLE=50,
    
    UCHAR_CHANGES_WHEN_LOWERCASED=51,
    
    UCHAR_CHANGES_WHEN_UPPERCASED=52,
    
    UCHAR_CHANGES_WHEN_TITLECASED=53,
    
    UCHAR_CHANGES_WHEN_CASEFOLDED=54,
    
    UCHAR_CHANGES_WHEN_CASEMAPPED=55,
    
    UCHAR_CHANGES_WHEN_NFKC_CASEFOLDED=56,
    
    UCHAR_BINARY_LIMIT=57,

    

    UCHAR_BIDI_CLASS=0x1000,
    
    UCHAR_INT_START=UCHAR_BIDI_CLASS,
    

    UCHAR_BLOCK=0x1001,
    

    UCHAR_CANONICAL_COMBINING_CLASS=0x1002,
    

    UCHAR_DECOMPOSITION_TYPE=0x1003,
    


    UCHAR_EAST_ASIAN_WIDTH=0x1004,
    

    UCHAR_GENERAL_CATEGORY=0x1005,
    

    UCHAR_JOINING_GROUP=0x1006,
    

    UCHAR_JOINING_TYPE=0x1007,
    

    UCHAR_LINE_BREAK=0x1008,
    

    UCHAR_NUMERIC_TYPE=0x1009,
    

    UCHAR_SCRIPT=0x100A,
    

    UCHAR_HANGUL_SYLLABLE_TYPE=0x100B,
    

    UCHAR_NFD_QUICK_CHECK=0x100C,
    

    UCHAR_NFKD_QUICK_CHECK=0x100D,
    

    UCHAR_NFC_QUICK_CHECK=0x100E,
    

    UCHAR_NFKC_QUICK_CHECK=0x100F,
    





    UCHAR_LEAD_CANONICAL_COMBINING_CLASS=0x1010,
    





    UCHAR_TRAIL_CANONICAL_COMBINING_CLASS=0x1011,
    



    UCHAR_GRAPHEME_CLUSTER_BREAK=0x1012,
    



    UCHAR_SENTENCE_BREAK=0x1013,
    



    UCHAR_WORD_BREAK=0x1014,
    
    UCHAR_INT_LIMIT=0x1015,

    







    UCHAR_GENERAL_CATEGORY_MASK=0x2000,
    
    UCHAR_MASK_START=UCHAR_GENERAL_CATEGORY_MASK,
    
    UCHAR_MASK_LIMIT=0x2001,

    

    UCHAR_NUMERIC_VALUE=0x3000,
    
    UCHAR_DOUBLE_START=UCHAR_NUMERIC_VALUE,
    
    UCHAR_DOUBLE_LIMIT=0x3001,

    

    UCHAR_AGE=0x4000,
    
    UCHAR_STRING_START=UCHAR_AGE,
    

    UCHAR_BIDI_MIRRORING_GLYPH=0x4001,
    

    UCHAR_CASE_FOLDING=0x4002,
    

    UCHAR_ISO_COMMENT=0x4003,
    

    UCHAR_LOWERCASE_MAPPING=0x4004,
    

    UCHAR_NAME=0x4005,
    

    UCHAR_SIMPLE_CASE_FOLDING=0x4006,
    

    UCHAR_SIMPLE_LOWERCASE_MAPPING=0x4007,
    

    UCHAR_SIMPLE_TITLECASE_MAPPING=0x4008,
    

    UCHAR_SIMPLE_UPPERCASE_MAPPING=0x4009,
    

    UCHAR_TITLECASE_MAPPING=0x400A,
    



    UCHAR_UNICODE_1_NAME=0x400B,
    

    UCHAR_UPPERCASE_MAPPING=0x400C,
    
    UCHAR_STRING_LIMIT=0x400D,
    






    UCHAR_SCRIPT_EXTENSIONS=0x7000,
    
    UCHAR_OTHER_PROPERTY_START=UCHAR_SCRIPT_EXTENSIONS,
    

    UCHAR_OTHER_PROPERTY_LIMIT=0x7001,
    
    UCHAR_INVALID_CODE = -1
} UProperty;






typedef enum UCharCategory
{
    






    
    U_UNASSIGNED              = 0,
    
    U_GENERAL_OTHER_TYPES     = 0,
    
    U_UPPERCASE_LETTER        = 1,
    
    U_LOWERCASE_LETTER        = 2,
    
    U_TITLECASE_LETTER        = 3,
    
    U_MODIFIER_LETTER         = 4,
    
    U_OTHER_LETTER            = 5,
    
    U_NON_SPACING_MARK        = 6,
    
    U_ENCLOSING_MARK          = 7,
    
    U_COMBINING_SPACING_MARK  = 8,
    
    U_DECIMAL_DIGIT_NUMBER    = 9,
    
    U_LETTER_NUMBER           = 10,
    
    U_OTHER_NUMBER            = 11,
    
    U_SPACE_SEPARATOR         = 12,
    
    U_LINE_SEPARATOR          = 13,
    
    U_PARAGRAPH_SEPARATOR     = 14,
    
    U_CONTROL_CHAR            = 15,
    
    U_FORMAT_CHAR             = 16,
    
    U_PRIVATE_USE_CHAR        = 17,
    
    U_SURROGATE               = 18,
    
    U_DASH_PUNCTUATION        = 19,
    
    U_START_PUNCTUATION       = 20,
    
    U_END_PUNCTUATION         = 21,
    
    U_CONNECTOR_PUNCTUATION   = 22,
    
    U_OTHER_PUNCTUATION       = 23,
    
    U_MATH_SYMBOL             = 24,
    
    U_CURRENCY_SYMBOL         = 25,
    
    U_MODIFIER_SYMBOL         = 26,
    
    U_OTHER_SYMBOL            = 27,
    
    U_INITIAL_PUNCTUATION     = 28,
    
    U_FINAL_PUNCTUATION       = 29,
    
    U_CHAR_CATEGORY_COUNT
} UCharCategory;















#define U_GC_CN_MASK    U_MASK(U_GENERAL_OTHER_TYPES)


#define U_GC_LU_MASK    U_MASK(U_UPPERCASE_LETTER)

#define U_GC_LL_MASK    U_MASK(U_LOWERCASE_LETTER)

#define U_GC_LT_MASK    U_MASK(U_TITLECASE_LETTER)

#define U_GC_LM_MASK    U_MASK(U_MODIFIER_LETTER)

#define U_GC_LO_MASK    U_MASK(U_OTHER_LETTER)


#define U_GC_MN_MASK    U_MASK(U_NON_SPACING_MARK)

#define U_GC_ME_MASK    U_MASK(U_ENCLOSING_MARK)

#define U_GC_MC_MASK    U_MASK(U_COMBINING_SPACING_MARK)


#define U_GC_ND_MASK    U_MASK(U_DECIMAL_DIGIT_NUMBER)

#define U_GC_NL_MASK    U_MASK(U_LETTER_NUMBER)

#define U_GC_NO_MASK    U_MASK(U_OTHER_NUMBER)


#define U_GC_ZS_MASK    U_MASK(U_SPACE_SEPARATOR)

#define U_GC_ZL_MASK    U_MASK(U_LINE_SEPARATOR)

#define U_GC_ZP_MASK    U_MASK(U_PARAGRAPH_SEPARATOR)


#define U_GC_CC_MASK    U_MASK(U_CONTROL_CHAR)

#define U_GC_CF_MASK    U_MASK(U_FORMAT_CHAR)

#define U_GC_CO_MASK    U_MASK(U_PRIVATE_USE_CHAR)

#define U_GC_CS_MASK    U_MASK(U_SURROGATE)


#define U_GC_PD_MASK    U_MASK(U_DASH_PUNCTUATION)

#define U_GC_PS_MASK    U_MASK(U_START_PUNCTUATION)

#define U_GC_PE_MASK    U_MASK(U_END_PUNCTUATION)

#define U_GC_PC_MASK    U_MASK(U_CONNECTOR_PUNCTUATION)

#define U_GC_PO_MASK    U_MASK(U_OTHER_PUNCTUATION)


#define U_GC_SM_MASK    U_MASK(U_MATH_SYMBOL)

#define U_GC_SC_MASK    U_MASK(U_CURRENCY_SYMBOL)

#define U_GC_SK_MASK    U_MASK(U_MODIFIER_SYMBOL)

#define U_GC_SO_MASK    U_MASK(U_OTHER_SYMBOL)


#define U_GC_PI_MASK    U_MASK(U_INITIAL_PUNCTUATION)

#define U_GC_PF_MASK    U_MASK(U_FINAL_PUNCTUATION)



#define U_GC_L_MASK \
            (U_GC_LU_MASK|U_GC_LL_MASK|U_GC_LT_MASK|U_GC_LM_MASK|U_GC_LO_MASK)


#define U_GC_LC_MASK \
            (U_GC_LU_MASK|U_GC_LL_MASK|U_GC_LT_MASK)


#define U_GC_M_MASK (U_GC_MN_MASK|U_GC_ME_MASK|U_GC_MC_MASK)


#define U_GC_N_MASK (U_GC_ND_MASK|U_GC_NL_MASK|U_GC_NO_MASK)


#define U_GC_Z_MASK (U_GC_ZS_MASK|U_GC_ZL_MASK|U_GC_ZP_MASK)


#define U_GC_C_MASK \
            (U_GC_CN_MASK|U_GC_CC_MASK|U_GC_CF_MASK|U_GC_CO_MASK|U_GC_CS_MASK)


#define U_GC_P_MASK \
            (U_GC_PD_MASK|U_GC_PS_MASK|U_GC_PE_MASK|U_GC_PC_MASK|U_GC_PO_MASK| \
             U_GC_PI_MASK|U_GC_PF_MASK)


#define U_GC_S_MASK (U_GC_SM_MASK|U_GC_SC_MASK|U_GC_SK_MASK|U_GC_SO_MASK)





typedef enum UCharDirection {
    






    
    U_LEFT_TO_RIGHT               = 0,
    
    U_RIGHT_TO_LEFT               = 1,
    
    U_EUROPEAN_NUMBER             = 2,
    
    U_EUROPEAN_NUMBER_SEPARATOR   = 3,
    
    U_EUROPEAN_NUMBER_TERMINATOR  = 4,
    
    U_ARABIC_NUMBER               = 5,
    
    U_COMMON_NUMBER_SEPARATOR     = 6,
    
    U_BLOCK_SEPARATOR             = 7,
    
    U_SEGMENT_SEPARATOR           = 8,
    
    U_WHITE_SPACE_NEUTRAL         = 9,
    
    U_OTHER_NEUTRAL               = 10,
    
    U_LEFT_TO_RIGHT_EMBEDDING     = 11,
    
    U_LEFT_TO_RIGHT_OVERRIDE      = 12,
    
    U_RIGHT_TO_LEFT_ARABIC        = 13,
    
    U_RIGHT_TO_LEFT_EMBEDDING     = 14,
    
    U_RIGHT_TO_LEFT_OVERRIDE      = 15,
    
    U_POP_DIRECTIONAL_FORMAT      = 16,
    
    U_DIR_NON_SPACING_MARK        = 17,
    
    U_BOUNDARY_NEUTRAL            = 18,
    
    U_CHAR_DIRECTION_COUNT
} UCharDirection;





enum UBlockCode {
    





    
    UBLOCK_NO_BLOCK = 0,  

    
    UBLOCK_BASIC_LATIN = 1, 

    
    UBLOCK_LATIN_1_SUPPLEMENT=2, 

    
    UBLOCK_LATIN_EXTENDED_A =3, 

    
    UBLOCK_LATIN_EXTENDED_B =4, 

    
    UBLOCK_IPA_EXTENSIONS =5, 

    
    UBLOCK_SPACING_MODIFIER_LETTERS =6, 

    
    UBLOCK_COMBINING_DIACRITICAL_MARKS =7, 

    



    UBLOCK_GREEK =8, 

    
    UBLOCK_CYRILLIC =9, 

    
    UBLOCK_ARMENIAN =10, 

    
    UBLOCK_HEBREW =11, 

    
    UBLOCK_ARABIC =12, 

    
    UBLOCK_SYRIAC =13, 

    
    UBLOCK_THAANA =14, 

    
    UBLOCK_DEVANAGARI =15, 

    
    UBLOCK_BENGALI =16, 

    
    UBLOCK_GURMUKHI =17, 

    
    UBLOCK_GUJARATI =18, 

    
    UBLOCK_ORIYA =19, 

    
    UBLOCK_TAMIL =20, 

    
    UBLOCK_TELUGU =21, 

    
    UBLOCK_KANNADA =22, 

    
    UBLOCK_MALAYALAM =23, 

    
    UBLOCK_SINHALA =24, 

    
    UBLOCK_THAI =25, 

    
    UBLOCK_LAO =26, 

    
    UBLOCK_TIBETAN =27, 

    
    UBLOCK_MYANMAR =28, 

    
    UBLOCK_GEORGIAN =29, 

    
    UBLOCK_HANGUL_JAMO =30, 

    
    UBLOCK_ETHIOPIC =31, 

    
    UBLOCK_CHEROKEE =32, 

    
    UBLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS =33, 

    
    UBLOCK_OGHAM =34, 

    
    UBLOCK_RUNIC =35, 

    
    UBLOCK_KHMER =36, 

    
    UBLOCK_MONGOLIAN =37, 

    
    UBLOCK_LATIN_EXTENDED_ADDITIONAL =38, 

    
    UBLOCK_GREEK_EXTENDED =39, 

    
    UBLOCK_GENERAL_PUNCTUATION =40, 

    
    UBLOCK_SUPERSCRIPTS_AND_SUBSCRIPTS =41, 

    
    UBLOCK_CURRENCY_SYMBOLS =42, 

    



    UBLOCK_COMBINING_MARKS_FOR_SYMBOLS =43, 

    
    UBLOCK_LETTERLIKE_SYMBOLS =44, 

    
    UBLOCK_NUMBER_FORMS =45, 

    
    UBLOCK_ARROWS =46, 

    
    UBLOCK_MATHEMATICAL_OPERATORS =47, 

    
    UBLOCK_MISCELLANEOUS_TECHNICAL =48, 

    
    UBLOCK_CONTROL_PICTURES =49, 

    
    UBLOCK_OPTICAL_CHARACTER_RECOGNITION =50, 

    
    UBLOCK_ENCLOSED_ALPHANUMERICS =51, 

    
    UBLOCK_BOX_DRAWING =52, 

    
    UBLOCK_BLOCK_ELEMENTS =53, 

    
    UBLOCK_GEOMETRIC_SHAPES =54, 

    
    UBLOCK_MISCELLANEOUS_SYMBOLS =55, 

    
    UBLOCK_DINGBATS =56, 

    
    UBLOCK_BRAILLE_PATTERNS =57, 

    
    UBLOCK_CJK_RADICALS_SUPPLEMENT =58, 

    
    UBLOCK_KANGXI_RADICALS =59, 

    
    UBLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS =60, 

    
    UBLOCK_CJK_SYMBOLS_AND_PUNCTUATION =61, 

    
    UBLOCK_HIRAGANA =62, 

    
    UBLOCK_KATAKANA =63, 

    
    UBLOCK_BOPOMOFO =64, 

    
    UBLOCK_HANGUL_COMPATIBILITY_JAMO =65, 

    
    UBLOCK_KANBUN =66, 

    
    UBLOCK_BOPOMOFO_EXTENDED =67, 

    
    UBLOCK_ENCLOSED_CJK_LETTERS_AND_MONTHS =68, 

    
    UBLOCK_CJK_COMPATIBILITY =69, 

    
    UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A =70, 

    
    UBLOCK_CJK_UNIFIED_IDEOGRAPHS =71, 

    
    UBLOCK_YI_SYLLABLES =72, 

    
    UBLOCK_YI_RADICALS =73, 

    
    UBLOCK_HANGUL_SYLLABLES =74, 

    
    UBLOCK_HIGH_SURROGATES =75, 

    
    UBLOCK_HIGH_PRIVATE_USE_SURROGATES =76, 

    
    UBLOCK_LOW_SURROGATES =77, 

    








    UBLOCK_PRIVATE_USE_AREA =78, 
    








    UBLOCK_PRIVATE_USE = UBLOCK_PRIVATE_USE_AREA,

    
    UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS =79, 

    
    UBLOCK_ALPHABETIC_PRESENTATION_FORMS =80, 

    
    UBLOCK_ARABIC_PRESENTATION_FORMS_A =81, 

    
    UBLOCK_COMBINING_HALF_MARKS =82, 

    
    UBLOCK_CJK_COMPATIBILITY_FORMS =83, 

    
    UBLOCK_SMALL_FORM_VARIANTS =84, 

    
    UBLOCK_ARABIC_PRESENTATION_FORMS_B =85, 

    
    UBLOCK_SPECIALS =86, 

    
    UBLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS =87, 

    

    
    UBLOCK_OLD_ITALIC = 88, 
    
    UBLOCK_GOTHIC = 89, 
    
    UBLOCK_DESERET = 90, 
    
    UBLOCK_BYZANTINE_MUSICAL_SYMBOLS = 91, 
    
    UBLOCK_MUSICAL_SYMBOLS = 92, 
    
    UBLOCK_MATHEMATICAL_ALPHANUMERIC_SYMBOLS = 93, 
    
    UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B  = 94, 
    
    UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT = 95, 
    
    UBLOCK_TAGS = 96, 

    

    
    UBLOCK_CYRILLIC_SUPPLEMENT = 97, 
    



    UBLOCK_CYRILLIC_SUPPLEMENTARY = UBLOCK_CYRILLIC_SUPPLEMENT, 
    
    UBLOCK_TAGALOG = 98, 
    
    UBLOCK_HANUNOO = 99, 
    
    UBLOCK_BUHID = 100, 
    
    UBLOCK_TAGBANWA = 101, 
    
    UBLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A = 102, 
    
    UBLOCK_SUPPLEMENTAL_ARROWS_A = 103, 
    
    UBLOCK_SUPPLEMENTAL_ARROWS_B = 104, 
    
    UBLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B = 105, 
    
    UBLOCK_SUPPLEMENTAL_MATHEMATICAL_OPERATORS = 106, 
    
    UBLOCK_KATAKANA_PHONETIC_EXTENSIONS = 107, 
    
    UBLOCK_VARIATION_SELECTORS = 108, 
    
    UBLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_A = 109, 
    
    UBLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_B = 110, 

    

    
    UBLOCK_LIMBU = 111, 
    
    UBLOCK_TAI_LE = 112, 
    
    UBLOCK_KHMER_SYMBOLS = 113, 
    
    UBLOCK_PHONETIC_EXTENSIONS = 114, 
    
    UBLOCK_MISCELLANEOUS_SYMBOLS_AND_ARROWS = 115, 
    
    UBLOCK_YIJING_HEXAGRAM_SYMBOLS = 116, 
    
    UBLOCK_LINEAR_B_SYLLABARY = 117, 
    
    UBLOCK_LINEAR_B_IDEOGRAMS = 118, 
    
    UBLOCK_AEGEAN_NUMBERS = 119, 
    
    UBLOCK_UGARITIC = 120, 
    
    UBLOCK_SHAVIAN = 121, 
    
    UBLOCK_OSMANYA = 122, 
    
    UBLOCK_CYPRIOT_SYLLABARY = 123, 
    
    UBLOCK_TAI_XUAN_JING_SYMBOLS = 124, 
    
    UBLOCK_VARIATION_SELECTORS_SUPPLEMENT = 125, 

    

    
    UBLOCK_ANCIENT_GREEK_MUSICAL_NOTATION = 126, 
    
    UBLOCK_ANCIENT_GREEK_NUMBERS = 127, 
    
    UBLOCK_ARABIC_SUPPLEMENT = 128, 
    
    UBLOCK_BUGINESE = 129, 
    
    UBLOCK_CJK_STROKES = 130, 
    
    UBLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT = 131, 
    
    UBLOCK_COPTIC = 132, 
    
    UBLOCK_ETHIOPIC_EXTENDED = 133, 
    
    UBLOCK_ETHIOPIC_SUPPLEMENT = 134, 
    
    UBLOCK_GEORGIAN_SUPPLEMENT = 135, 
    
    UBLOCK_GLAGOLITIC = 136, 
    
    UBLOCK_KHAROSHTHI = 137, 
    
    UBLOCK_MODIFIER_TONE_LETTERS = 138, 
    
    UBLOCK_NEW_TAI_LUE = 139, 
    
    UBLOCK_OLD_PERSIAN = 140, 
    
    UBLOCK_PHONETIC_EXTENSIONS_SUPPLEMENT = 141, 
    
    UBLOCK_SUPPLEMENTAL_PUNCTUATION = 142, 
    
    UBLOCK_SYLOTI_NAGRI = 143, 
    
    UBLOCK_TIFINAGH = 144, 
    
    UBLOCK_VERTICAL_FORMS = 145, 

    

    
    UBLOCK_NKO = 146, 
    
    UBLOCK_BALINESE = 147, 
    
    UBLOCK_LATIN_EXTENDED_C = 148, 
    
    UBLOCK_LATIN_EXTENDED_D = 149, 
    
    UBLOCK_PHAGS_PA = 150, 
    
    UBLOCK_PHOENICIAN = 151, 
    
    UBLOCK_CUNEIFORM = 152, 
    
    UBLOCK_CUNEIFORM_NUMBERS_AND_PUNCTUATION = 153, 
    
    UBLOCK_COUNTING_ROD_NUMERALS = 154, 

    

    
    UBLOCK_SUNDANESE = 155, 
    
    UBLOCK_LEPCHA = 156, 
    
    UBLOCK_OL_CHIKI = 157, 
    
    UBLOCK_CYRILLIC_EXTENDED_A = 158, 
    
    UBLOCK_VAI = 159, 
    
    UBLOCK_CYRILLIC_EXTENDED_B = 160, 
    
    UBLOCK_SAURASHTRA = 161, 
    
    UBLOCK_KAYAH_LI = 162, 
    
    UBLOCK_REJANG = 163, 
    
    UBLOCK_CHAM = 164, 
    
    UBLOCK_ANCIENT_SYMBOLS = 165, 
    
    UBLOCK_PHAISTOS_DISC = 166, 
    
    UBLOCK_LYCIAN = 167, 
    
    UBLOCK_CARIAN = 168, 
    
    UBLOCK_LYDIAN = 169, 
    
    UBLOCK_MAHJONG_TILES = 170, 
    
    UBLOCK_DOMINO_TILES = 171, 

    

    
    UBLOCK_SAMARITAN = 172, 
    
    UBLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED = 173, 
    
    UBLOCK_TAI_THAM = 174, 
    
    UBLOCK_VEDIC_EXTENSIONS = 175, 
    
    UBLOCK_LISU = 176, 
    
    UBLOCK_BAMUM = 177, 
    
    UBLOCK_COMMON_INDIC_NUMBER_FORMS = 178, 
    
    UBLOCK_DEVANAGARI_EXTENDED = 179, 
    
    UBLOCK_HANGUL_JAMO_EXTENDED_A = 180, 
    
    UBLOCK_JAVANESE = 181, 
    
    UBLOCK_MYANMAR_EXTENDED_A = 182, 
    
    UBLOCK_TAI_VIET = 183, 
    
    UBLOCK_MEETEI_MAYEK = 184, 
    
    UBLOCK_HANGUL_JAMO_EXTENDED_B = 185, 
    
    UBLOCK_IMPERIAL_ARAMAIC = 186, 
    
    UBLOCK_OLD_SOUTH_ARABIAN = 187, 
    
    UBLOCK_AVESTAN = 188, 
    
    UBLOCK_INSCRIPTIONAL_PARTHIAN = 189, 
    
    UBLOCK_INSCRIPTIONAL_PAHLAVI = 190, 
    
    UBLOCK_OLD_TURKIC = 191, 
    
    UBLOCK_RUMI_NUMERAL_SYMBOLS = 192, 
    
    UBLOCK_KAITHI = 193, 
    
    UBLOCK_EGYPTIAN_HIEROGLYPHS = 194, 
    
    UBLOCK_ENCLOSED_ALPHANUMERIC_SUPPLEMENT = 195, 
    
    UBLOCK_ENCLOSED_IDEOGRAPHIC_SUPPLEMENT = 196, 
    
    UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C = 197, 

    

    
    UBLOCK_MANDAIC = 198, 
    
    UBLOCK_BATAK = 199, 
    
    UBLOCK_ETHIOPIC_EXTENDED_A = 200, 
    
    UBLOCK_BRAHMI = 201, 
    
    UBLOCK_BAMUM_SUPPLEMENT = 202, 
    
    UBLOCK_KANA_SUPPLEMENT = 203, 
    
    UBLOCK_PLAYING_CARDS = 204, 
    
    UBLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS = 205, 
    
    UBLOCK_EMOTICONS = 206, 
    
    UBLOCK_TRANSPORT_AND_MAP_SYMBOLS = 207, 
    
    UBLOCK_ALCHEMICAL_SYMBOLS = 208, 
    
    UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D = 209, 

    

    
    UBLOCK_ARABIC_EXTENDED_A = 210, 
    
    UBLOCK_ARABIC_MATHEMATICAL_ALPHABETIC_SYMBOLS = 211, 
    
    UBLOCK_CHAKMA = 212, 
    
    UBLOCK_MEETEI_MAYEK_EXTENSIONS = 213, 
    
    UBLOCK_MEROITIC_CURSIVE = 214, 
    
    UBLOCK_MEROITIC_HIEROGLYPHS = 215, 
    
    UBLOCK_MIAO = 216, 
    
    UBLOCK_SHARADA = 217, 
    
    UBLOCK_SORA_SOMPENG = 218, 
    
    UBLOCK_SUNDANESE_SUPPLEMENT = 219, 
    
    UBLOCK_TAKRI = 220, 

    
    UBLOCK_COUNT = 221,

    
    UBLOCK_INVALID_CODE=-1
};


typedef enum UBlockCode UBlockCode;








typedef enum UEastAsianWidth {
    





    U_EA_NEUTRAL,   
    U_EA_AMBIGUOUS, 
    U_EA_HALFWIDTH, 
    U_EA_FULLWIDTH, 
    U_EA_NARROW,    
    U_EA_WIDE,      
    U_EA_COUNT
} UEastAsianWidth;












typedef enum UCharNameChoice {
    
    U_UNICODE_CHAR_NAME,
    




    U_UNICODE_10_CHAR_NAME,
    
    U_EXTENDED_CHAR_NAME,
    
    U_CHAR_NAME_ALIAS,
    
    U_CHAR_NAME_CHOICE_COUNT
} UCharNameChoice;














typedef enum UPropertyNameChoice {
    U_SHORT_PROPERTY_NAME,
    U_LONG_PROPERTY_NAME,
    U_PROPERTY_NAME_CHOICE_COUNT
} UPropertyNameChoice;







typedef enum UDecompositionType {
    





    U_DT_NONE,              
    U_DT_CANONICAL,         
    U_DT_COMPAT,            
    U_DT_CIRCLE,            
    U_DT_FINAL,             
    U_DT_FONT,              
    U_DT_FRACTION,          
    U_DT_INITIAL,           
    U_DT_ISOLATED,          
    U_DT_MEDIAL,            
    U_DT_NARROW,            
    U_DT_NOBREAK,           
    U_DT_SMALL,             
    U_DT_SQUARE,            
    U_DT_SUB,               
    U_DT_SUPER,             
    U_DT_VERTICAL,          
    U_DT_WIDE,              
    U_DT_COUNT 
} UDecompositionType;







typedef enum UJoiningType {
    





    U_JT_NON_JOINING,       
    U_JT_JOIN_CAUSING,      
    U_JT_DUAL_JOINING,      
    U_JT_LEFT_JOINING,      
    U_JT_RIGHT_JOINING,     
    U_JT_TRANSPARENT,       
    U_JT_COUNT 
} UJoiningType;







typedef enum UJoiningGroup {
    





    U_JG_NO_JOINING_GROUP,
    U_JG_AIN,
    U_JG_ALAPH,
    U_JG_ALEF,
    U_JG_BEH,
    U_JG_BETH,
    U_JG_DAL,
    U_JG_DALATH_RISH,
    U_JG_E,
    U_JG_FEH,
    U_JG_FINAL_SEMKATH,
    U_JG_GAF,
    U_JG_GAMAL,
    U_JG_HAH,
    U_JG_TEH_MARBUTA_GOAL,  
    U_JG_HAMZA_ON_HEH_GOAL=U_JG_TEH_MARBUTA_GOAL,
    U_JG_HE,
    U_JG_HEH,
    U_JG_HEH_GOAL,
    U_JG_HETH,
    U_JG_KAF,
    U_JG_KAPH,
    U_JG_KNOTTED_HEH,
    U_JG_LAM,
    U_JG_LAMADH,
    U_JG_MEEM,
    U_JG_MIM,
    U_JG_NOON,
    U_JG_NUN,
    U_JG_PE,
    U_JG_QAF,
    U_JG_QAPH,
    U_JG_REH,
    U_JG_REVERSED_PE,
    U_JG_SAD,
    U_JG_SADHE,
    U_JG_SEEN,
    U_JG_SEMKATH,
    U_JG_SHIN,
    U_JG_SWASH_KAF,
    U_JG_SYRIAC_WAW,
    U_JG_TAH,
    U_JG_TAW,
    U_JG_TEH_MARBUTA,
    U_JG_TETH,
    U_JG_WAW,
    U_JG_YEH,
    U_JG_YEH_BARREE,
    U_JG_YEH_WITH_TAIL,
    U_JG_YUDH,
    U_JG_YUDH_HE,
    U_JG_ZAIN,
    U_JG_FE,        
    U_JG_KHAPH,     
    U_JG_ZHAIN,     
    U_JG_BURUSHASKI_YEH_BARREE, 
    U_JG_FARSI_YEH, 
    U_JG_NYA,       
    U_JG_ROHINGYA_YEH,  
    U_JG_COUNT
} UJoiningGroup;







typedef enum UGraphemeClusterBreak {
    





    U_GCB_OTHER = 0,            
    U_GCB_CONTROL = 1,          
    U_GCB_CR = 2,               
    U_GCB_EXTEND = 3,           
    U_GCB_L = 4,                
    U_GCB_LF = 5,               
    U_GCB_LV = 6,               
    U_GCB_LVT = 7,              
    U_GCB_T = 8,                
    U_GCB_V = 9,                
    U_GCB_SPACING_MARK = 10,     
    U_GCB_PREPEND = 11,         
    U_GCB_REGIONAL_INDICATOR = 12,   
    U_GCB_COUNT = 13
} UGraphemeClusterBreak;








typedef enum UWordBreakValues {
    





    U_WB_OTHER = 0,             
    U_WB_ALETTER = 1,           
    U_WB_FORMAT = 2,            
    U_WB_KATAKANA = 3,          
    U_WB_MIDLETTER = 4,         
    U_WB_MIDNUM = 5,            
    U_WB_NUMERIC = 6,           
    U_WB_EXTENDNUMLET = 7,      
    U_WB_CR = 8,                 
    U_WB_EXTEND = 9,            
    U_WB_LF = 10,               
    U_WB_MIDNUMLET =11,         
    U_WB_NEWLINE =12,           
    U_WB_REGIONAL_INDICATOR = 13,    
    U_WB_COUNT = 14
} UWordBreakValues;







typedef enum USentenceBreak {
    





    U_SB_OTHER = 0,             
    U_SB_ATERM = 1,             
    U_SB_CLOSE = 2,             
    U_SB_FORMAT = 3,            
    U_SB_LOWER = 4,             
    U_SB_NUMERIC = 5,           
    U_SB_OLETTER = 6,           
    U_SB_SEP = 7,               
    U_SB_SP = 8,                
    U_SB_STERM = 9,             
    U_SB_UPPER = 10,            
    U_SB_CR = 11,                
    U_SB_EXTEND = 12,           
    U_SB_LF = 13,               
    U_SB_SCONTINUE = 14,        
    U_SB_COUNT = 15
} USentenceBreak;







typedef enum ULineBreak {
    





    U_LB_UNKNOWN = 0,           
    U_LB_AMBIGUOUS = 1,         
    U_LB_ALPHABETIC = 2,        
    U_LB_BREAK_BOTH = 3,        
    U_LB_BREAK_AFTER = 4,       
    U_LB_BREAK_BEFORE = 5,      
    U_LB_MANDATORY_BREAK = 6,   
    U_LB_CONTINGENT_BREAK = 7,  
    U_LB_CLOSE_PUNCTUATION = 8, 
    U_LB_COMBINING_MARK = 9,    
    U_LB_CARRIAGE_RETURN = 10,   
    U_LB_EXCLAMATION = 11,       
    U_LB_GLUE = 12,              
    U_LB_HYPHEN = 13,            
    U_LB_IDEOGRAPHIC = 14,       
    
    U_LB_INSEPARABLE = 15,       
    U_LB_INSEPERABLE = U_LB_INSEPARABLE,
    U_LB_INFIX_NUMERIC = 16,     
    U_LB_LINE_FEED = 17,         
    U_LB_NONSTARTER = 18,        
    U_LB_NUMERIC = 19,           
    U_LB_OPEN_PUNCTUATION = 20,  
    U_LB_POSTFIX_NUMERIC = 21,   
    U_LB_PREFIX_NUMERIC = 22,    
    U_LB_QUOTATION = 23,         
    U_LB_COMPLEX_CONTEXT = 24,   
    U_LB_SURROGATE = 25,         
    U_LB_SPACE = 26,             
    U_LB_BREAK_SYMBOLS = 27,     
    U_LB_ZWSPACE = 28,           
    U_LB_NEXT_LINE = 29,          
    U_LB_WORD_JOINER = 30,       
    U_LB_H2 = 31,                 
    U_LB_H3 = 32,                
    U_LB_JL = 33,                
    U_LB_JT = 34,                
    U_LB_JV = 35,                
    U_LB_CLOSE_PARENTHESIS = 36,  
    U_LB_CONDITIONAL_JAPANESE_STARTER = 37, 
    U_LB_HEBREW_LETTER = 38,      
    U_LB_REGIONAL_INDICATOR = 39, 
    U_LB_COUNT = 40
} ULineBreak;







typedef enum UNumericType {
    





    U_NT_NONE,              
    U_NT_DECIMAL,           
    U_NT_DIGIT,             
    U_NT_NUMERIC,           
    U_NT_COUNT
} UNumericType;







typedef enum UHangulSyllableType {
    





    U_HST_NOT_APPLICABLE,   
    U_HST_LEADING_JAMO,     
    U_HST_VOWEL_JAMO,       
    U_HST_TRAILING_JAMO,    
    U_HST_LV_SYLLABLE,      
    U_HST_LVT_SYLLABLE,     
    U_HST_COUNT
} UHangulSyllableType;



























U_STABLE UBool U_EXPORT2
u_hasBinaryProperty(UChar32 c, UProperty which);













U_STABLE UBool U_EXPORT2
u_isUAlphabetic(UChar32 c);













U_STABLE UBool U_EXPORT2
u_isULowercase(UChar32 c);













U_STABLE UBool U_EXPORT2
u_isUUppercase(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isUWhiteSpace(UChar32 c);






































U_STABLE int32_t U_EXPORT2
u_getIntPropertyValue(UChar32 c, UProperty which);



















U_STABLE int32_t U_EXPORT2
u_getIntPropertyMinValue(UProperty which);



























U_STABLE int32_t U_EXPORT2
u_getIntPropertyMaxValue(UProperty which);























U_STABLE double U_EXPORT2
u_getNumericValue(UChar32 c);








#define U_NO_NUMERIC_VALUE ((double)-123456789.)
























U_STABLE UBool U_EXPORT2
u_islower(UChar32 c);

























U_STABLE UBool U_EXPORT2
u_isupper(UChar32 c);















U_STABLE UBool U_EXPORT2
u_istitle(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isdigit(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isalpha(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isalnum(UChar32 c);





















U_STABLE UBool U_EXPORT2
u_isxdigit(UChar32 c);














U_STABLE UBool U_EXPORT2
u_ispunct(UChar32 c);

















U_STABLE UBool U_EXPORT2
u_isgraph(UChar32 c);



























U_STABLE UBool U_EXPORT2
u_isblank(UChar32 c);























U_STABLE UBool U_EXPORT2
u_isdefined(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isspace(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isJavaSpaceChar(UChar32 c);






































U_STABLE UBool U_EXPORT2
u_isWhitespace(UChar32 c);






















U_STABLE UBool U_EXPORT2
u_iscntrl(UChar32 c);













U_STABLE UBool U_EXPORT2
u_isISOControl(UChar32 c);
















U_STABLE UBool U_EXPORT2
u_isprint(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isbase(UChar32 c);

















U_STABLE UCharDirection U_EXPORT2
u_charDirection(UChar32 c);
















U_STABLE UBool U_EXPORT2
u_isMirrored(UChar32 c);




















U_STABLE UChar32 U_EXPORT2
u_charMirror(UChar32 c);












U_STABLE int8_t U_EXPORT2
u_charType(UChar32 c);














#define U_GET_GC_MASK(c) U_MASK(u_charType(c))


















typedef UBool U_CALLCONV
UCharEnumTypeRange(const void *context, UChar32 start, UChar32 limit, UCharCategory type);




















U_STABLE void U_EXPORT2
u_enumCharTypes(UCharEnumTypeRange *enumRange, const void *context);

#if !UCONFIG_NO_NORMALIZATION








U_STABLE uint8_t U_EXPORT2
u_getCombiningClass(UChar32 c);

#endif
























U_STABLE int32_t U_EXPORT2
u_charDigitValue(UChar32 c);










U_STABLE UBlockCode U_EXPORT2
ublock_getCode(UChar32 c);

































U_STABLE int32_t U_EXPORT2
u_charName(UChar32 code, UCharNameChoice nameChoice,
           char *buffer, int32_t bufferLength,
           UErrorCode *pErrorCode);



















U_STABLE int32_t U_EXPORT2
u_getISOComment(UChar32 c,
                char *dest, int32_t destCapacity,
                UErrorCode *pErrorCode);





















U_STABLE UChar32 U_EXPORT2
u_charFromName(UCharNameChoice nameChoice,
               const char *name,
               UErrorCode *pErrorCode);


















typedef UBool U_CALLCONV UEnumCharNamesFn(void *context,
                               UChar32 code,
                               UCharNameChoice nameChoice,
                               const char *name,
                               int32_t length);






















U_STABLE void U_EXPORT2
u_enumCharNames(UChar32 start, UChar32 limit,
                UEnumCharNamesFn *fn,
                void *context,
                UCharNameChoice nameChoice,
                UErrorCode *pErrorCode);
































U_STABLE const char* U_EXPORT2
u_getPropertyName(UProperty property,
                  UPropertyNameChoice nameChoice);




















U_STABLE UProperty U_EXPORT2
u_getPropertyEnum(const char* alias);
















































U_STABLE const char* U_EXPORT2
u_getPropertyValueName(UProperty property,
                       int32_t value,
                       UPropertyNameChoice nameChoice);
































U_STABLE int32_t U_EXPORT2
u_getPropertyValueEnum(UProperty property,
                       const char* alias);


















U_STABLE UBool U_EXPORT2
u_isIDStart(UChar32 c);






















U_STABLE UBool U_EXPORT2
u_isIDPart(UChar32 c);





















U_STABLE UBool U_EXPORT2
u_isIDIgnorable(UChar32 c);

















U_STABLE UBool U_EXPORT2
u_isJavaIDStart(UChar32 c);



















U_STABLE UBool U_EXPORT2
u_isJavaIDPart(UChar32 c);























U_STABLE UChar32 U_EXPORT2
u_tolower(UChar32 c);























U_STABLE UChar32 U_EXPORT2
u_toupper(UChar32 c);























U_STABLE UChar32 U_EXPORT2
u_totitle(UChar32 c);


#define U_FOLD_CASE_DEFAULT 0

















#define U_FOLD_CASE_EXCLUDE_SPECIAL_I 1























U_STABLE UChar32 U_EXPORT2
u_foldCase(UChar32 c, uint32_t options);







































U_STABLE int32_t U_EXPORT2
u_digit(UChar32 ch, int8_t radix);





























U_STABLE UChar32 U_EXPORT2
u_forDigit(int32_t digit, int8_t radix);















U_STABLE void U_EXPORT2
u_charAge(UChar32 c, UVersionInfo versionArray);












U_STABLE void U_EXPORT2
u_getUnicodeVersion(UVersionInfo versionArray);

#if !UCONFIG_NO_NORMALIZATION





















U_STABLE int32_t U_EXPORT2
u_getFC_NFKC_Closure(UChar32 c, UChar *dest, int32_t destCapacity, UErrorCode *pErrorCode);

#endif


U_CDECL_END

#endif 

