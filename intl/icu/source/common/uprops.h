


















#ifndef __UPROPS_H__
#define __UPROPS_H__

#include "unicode/utypes.h"
#include "unicode/uset.h"
#include "uset_imp.h"
#include "udataswp.h"


enum {
    UPROPS_PROPS32_INDEX,
    UPROPS_EXCEPTIONS_INDEX,
    UPROPS_EXCEPTIONS_TOP_INDEX,

    UPROPS_ADDITIONAL_TRIE_INDEX,
    UPROPS_ADDITIONAL_VECTORS_INDEX,
    UPROPS_ADDITIONAL_VECTORS_COLUMNS_INDEX,

    UPROPS_SCRIPT_EXTENSIONS_INDEX,

    UPROPS_RESERVED_INDEX_7,
    UPROPS_RESERVED_INDEX_8,

    
    UPROPS_DATA_TOP_INDEX,

    
    UPROPS_MAX_VALUES_INDEX=10,
    
    UPROPS_MAX_VALUES_2_INDEX,

    UPROPS_INDEX_COUNT=16
};


enum {
    
    
    UPROPS_NUMERIC_TYPE_VALUE_SHIFT=6                       
};

#define GET_CATEGORY(props) ((props)&0x1f)
#define CAT_MASK(props) U_MASK(GET_CATEGORY(props))

#define GET_NUMERIC_TYPE_VALUE(props) ((props)>>UPROPS_NUMERIC_TYPE_VALUE_SHIFT)


enum {
    
    UPROPS_NTV_NONE=0,
    
    UPROPS_NTV_DECIMAL_START=1,
    
    UPROPS_NTV_DIGIT_START=11,
    
    UPROPS_NTV_NUMERIC_START=21,
    
    UPROPS_NTV_FRACTION_START=0xb0,
    




    UPROPS_NTV_LARGE_START=0x1e0,
    



    UPROPS_NTV_BASE60_START=0x300,
    
    UPROPS_NTV_RESERVED_START=UPROPS_NTV_BASE60_START+36,  

    UPROPS_NTV_MAX_SMALL_INT=UPROPS_NTV_FRACTION_START-UPROPS_NTV_NUMERIC_START-1
};

#define UPROPS_NTV_GET_TYPE(ntv) \
    ((ntv==UPROPS_NTV_NONE) ? U_NT_NONE : \
    (ntv<UPROPS_NTV_DIGIT_START) ?  U_NT_DECIMAL : \
    (ntv<UPROPS_NTV_NUMERIC_START) ? U_NT_DIGIT : \
    U_NT_NUMERIC)


#define UPROPS_VECTOR_WORDS     3

















#define UPROPS_AGE_MASK         0xff000000
#define UPROPS_AGE_SHIFT        24


#define UPROPS_SCRIPT_X_MASK    0x00c000ff
#define UPROPS_SCRIPT_X_SHIFT   22

#define UPROPS_EA_MASK          0x000e0000
#define UPROPS_EA_SHIFT         17

#define UPROPS_BLOCK_MASK       0x0001ff00
#define UPROPS_BLOCK_SHIFT      8

#define UPROPS_SCRIPT_MASK      0x000000ff


#define UPROPS_SCRIPT_X_WITH_COMMON     0x400000
#define UPROPS_SCRIPT_X_WITH_INHERITED  0x800000
#define UPROPS_SCRIPT_X_WITH_OTHER      0xc00000












enum {
    UPROPS_WHITE_SPACE,
    UPROPS_DASH,
    UPROPS_HYPHEN,
    UPROPS_QUOTATION_MARK,
    UPROPS_TERMINAL_PUNCTUATION,
    UPROPS_MATH,
    UPROPS_HEX_DIGIT,
    UPROPS_ASCII_HEX_DIGIT,
    UPROPS_ALPHABETIC,
    UPROPS_IDEOGRAPHIC,
    UPROPS_DIACRITIC,
    UPROPS_EXTENDER,
    UPROPS_NONCHARACTER_CODE_POINT,
    UPROPS_GRAPHEME_EXTEND,
    UPROPS_GRAPHEME_LINK,
    UPROPS_IDS_BINARY_OPERATOR,
    UPROPS_IDS_TRINARY_OPERATOR,
    UPROPS_RADICAL,
    UPROPS_UNIFIED_IDEOGRAPH,
    UPROPS_DEFAULT_IGNORABLE_CODE_POINT,
    UPROPS_DEPRECATED,
    UPROPS_LOGICAL_ORDER_EXCEPTION,
    UPROPS_XID_START,
    UPROPS_XID_CONTINUE,
    UPROPS_ID_START,                            
    UPROPS_ID_CONTINUE,
    UPROPS_GRAPHEME_BASE,
    UPROPS_S_TERM,                              
    UPROPS_VARIATION_SELECTOR,
    UPROPS_PATTERN_SYNTAX,                      
    UPROPS_PATTERN_WHITE_SPACE,
    UPROPS_RESERVED,                            
    UPROPS_BINARY_1_TOP                         
};











#define UPROPS_LB_MASK          0x03f00000
#define UPROPS_LB_SHIFT         20

#define UPROPS_SB_MASK          0x000f8000
#define UPROPS_SB_SHIFT         15

#define UPROPS_WB_MASK          0x00007c00
#define UPROPS_WB_SHIFT         10

#define UPROPS_GCB_MASK         0x000003e0
#define UPROPS_GCB_SHIFT        5

#define UPROPS_DT_MASK          0x0000001f





U_CFUNC uint32_t
u_getMainProperties(UChar32 c);






U_CFUNC uint32_t
u_getUnicodeProperties(UChar32 c, int32_t column);














U_CFUNC int32_t
uprv_getMaxValues(int32_t column);





U_CFUNC UBool
u_isalnumPOSIX(UChar32 c);








U_CFUNC UBool
u_isgraphPOSIX(UChar32 c);






U_CFUNC UBool
u_isprintPOSIX(UChar32 c);


#define FLAG(n) ((uint32_t)1<<(n))


#define _Cn     FLAG(U_GENERAL_OTHER_TYPES)
#define _Lu     FLAG(U_UPPERCASE_LETTER)
#define _Ll     FLAG(U_LOWERCASE_LETTER)
#define _Lt     FLAG(U_TITLECASE_LETTER)
#define _Lm     FLAG(U_MODIFIER_LETTER)

#define _Mn     FLAG(U_NON_SPACING_MARK)
#define _Me     FLAG(U_ENCLOSING_MARK)
#define _Mc     FLAG(U_COMBINING_SPACING_MARK)
#define _Nd     FLAG(U_DECIMAL_DIGIT_NUMBER)
#define _Nl     FLAG(U_LETTER_NUMBER)
#define _No     FLAG(U_OTHER_NUMBER)
#define _Zs     FLAG(U_SPACE_SEPARATOR)
#define _Zl     FLAG(U_LINE_SEPARATOR)
#define _Zp     FLAG(U_PARAGRAPH_SEPARATOR)
#define _Cc     FLAG(U_CONTROL_CHAR)
#define _Cf     FLAG(U_FORMAT_CHAR)
#define _Co     FLAG(U_PRIVATE_USE_CHAR)
#define _Cs     FLAG(U_SURROGATE)
#define _Pd     FLAG(U_DASH_PUNCTUATION)
#define _Ps     FLAG(U_START_PUNCTUATION)


#define _Po     FLAG(U_OTHER_PUNCTUATION)
#define _Sm     FLAG(U_MATH_SYMBOL)
#define _Sc     FLAG(U_CURRENCY_SYMBOL)
#define _Sk     FLAG(U_MODIFIER_SYMBOL)
#define _So     FLAG(U_OTHER_SYMBOL)
#define _Pi     FLAG(U_INITIAL_PUNCTUATION)



enum {
    TAB     =0x0009,
    LF      =0x000a,
    FF      =0x000c,
    CR      =0x000d,
    U_A     =0x0041,
    U_F     =0x0046,
    U_Z     =0x005a,
    U_a     =0x0061,
    U_f     =0x0066,
    U_z     =0x007a,
    DEL     =0x007f,
    NL      =0x0085,
    NBSP    =0x00a0,
    CGJ     =0x034f,
    FIGURESP=0x2007,
    HAIRSP  =0x200a,
    ZWNJ    =0x200c,
    ZWJ     =0x200d,
    RLM     =0x200f,
    NNBSP   =0x202f,
    WJ      =0x2060,
    INHSWAP =0x206a,
    NOMDIG  =0x206f,
    U_FW_A  =0xff21,
    U_FW_F  =0xff26,
    U_FW_Z  =0xff3a,
    U_FW_a  =0xff41,
    U_FW_f  =0xff46,
    U_FW_z  =0xff5a,
    ZWNBSP  =0xfeff
};





U_CAPI int32_t U_EXPORT2
uprv_getMaxCharNameLength(void);







U_CAPI void U_EXPORT2
uprv_getCharNameCharacters(const USetAdder *sa);






enum UPropertySource {
    
    UPROPS_SRC_NONE,
    
    UPROPS_SRC_CHAR,
    
    UPROPS_SRC_PROPSVEC,
    
    UPROPS_SRC_NAMES,
    
    UPROPS_SRC_CASE,
    
    UPROPS_SRC_BIDI,
    
    UPROPS_SRC_CHAR_AND_PROPSVEC,
    
    UPROPS_SRC_CASE_AND_NORM,
    
    UPROPS_SRC_NFC,
    
    UPROPS_SRC_NFKC,
    
    UPROPS_SRC_NFKC_CF,
    
    UPROPS_SRC_NFC_CANON_ITER,
    
    UPROPS_SRC_COUNT
};
typedef enum UPropertySource UPropertySource;





U_CFUNC UPropertySource U_EXPORT2
uprops_getSource(UProperty which);






U_CFUNC void U_EXPORT2
uchar_addPropertyStarts(const USetAdder *sa, UErrorCode *pErrorCode);






U_CFUNC void U_EXPORT2
upropsvec_addPropertyStarts(const USetAdder *sa, UErrorCode *pErrorCode);

















U_CAPI int32_t U_EXPORT2
uchar_swapNames(const UDataSwapper *ds,
                const void *inData, int32_t length, void *outData,
                UErrorCode *pErrorCode);

#ifdef __cplusplus

U_NAMESPACE_BEGIN

class UnicodeSet;


U_CFUNC UnicodeSet *
uniset_getUnicode32Instance(UErrorCode &errorCode);

U_NAMESPACE_END

#endif

#endif
