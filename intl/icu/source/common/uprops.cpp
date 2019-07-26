






















#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/unorm2.h"
#include "unicode/uscript.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "normalizer2impl.h"
#include "ucln_cmn.h"
#include "umutex.h"
#include "ubidi_props.h"
#include "uprops.h"
#include "ucase.h"
#include "ustr_imp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

U_NAMESPACE_USE

#define GET_BIDI_PROPS() ubidi_getSingleton()



struct BinaryProperty;

typedef UBool BinaryPropertyContains(const BinaryProperty &prop, UChar32 c, UProperty which);

struct BinaryProperty {
    int32_t column;  
    uint32_t mask;
    BinaryPropertyContains *contains;
};

static UBool defaultContains(const BinaryProperty &prop, UChar32 c, UProperty ) {
    
    return (u_getUnicodeProperties(c, prop.column)&prop.mask)!=0;
}

static UBool caseBinaryPropertyContains(const BinaryProperty &, UChar32 c, UProperty which) {
    return ucase_hasBinaryProperty(c, which);
}

static UBool isBidiControl(const BinaryProperty &, UChar32 c, UProperty ) {
    return ubidi_isBidiControl(GET_BIDI_PROPS(), c);
}

static UBool isMirrored(const BinaryProperty &, UChar32 c, UProperty ) {
    return ubidi_isMirrored(GET_BIDI_PROPS(), c);
}

static UBool isJoinControl(const BinaryProperty &, UChar32 c, UProperty ) {
    return ubidi_isJoinControl(GET_BIDI_PROPS(), c);
}

#if UCONFIG_NO_NORMALIZATION
static UBool hasFullCompositionExclusion(const BinaryProperty &, UChar32, UProperty) {
    return FALSE;
}
#else
static UBool hasFullCompositionExclusion(const BinaryProperty &, UChar32 c, UProperty ) {
    
    UErrorCode errorCode=U_ZERO_ERROR;
    const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(errorCode);
    return U_SUCCESS(errorCode) && impl->isCompNo(impl->getNorm16(c));
}
#endif


#if UCONFIG_NO_NORMALIZATION
static UBool isNormInert(const BinaryProperty &, UChar32, UProperty) {
    return FALSE;
}
#else
static UBool isNormInert(const BinaryProperty &, UChar32 c, UProperty which) {
    UErrorCode errorCode=U_ZERO_ERROR;
    const Normalizer2 *norm2=Normalizer2Factory::getInstance(
        (UNormalizationMode)(which-UCHAR_NFD_INERT+UNORM_NFD), errorCode);
    return U_SUCCESS(errorCode) && norm2->isInert(c);
}
#endif

#if UCONFIG_NO_NORMALIZATION
static UBool changesWhenCasefolded(const BinaryProperty &, UChar32, UProperty) {
    return FALSE;
}
#else
static UBool changesWhenCasefolded(const BinaryProperty &, UChar32 c, UProperty ) {
    UnicodeString nfd;
    UErrorCode errorCode=U_ZERO_ERROR;
    const Normalizer2 *nfcNorm2=Normalizer2Factory::getNFCInstance(errorCode);
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    if(nfcNorm2->getDecomposition(c, nfd)) {
        
        if(nfd.length()==1) {
            c=nfd[0];  
        } else if(nfd.length()<=U16_MAX_LENGTH &&
                  nfd.length()==U16_LENGTH(c=nfd.char32At(0))
        ) {
            
        } else {
            c=U_SENTINEL;
        }
    } else if(c<0) {
        return FALSE;  
    }
    if(c>=0) {
        
        const UCaseProps *csp=ucase_getSingleton();
        const UChar *resultString;
        return (UBool)(ucase_toFullFolding(csp, c, &resultString, U_FOLD_CASE_DEFAULT)>=0);
    } else {
        
        UChar dest[2*UCASE_MAX_STRING_LENGTH];
        int32_t destLength;
        destLength=u_strFoldCase(dest, LENGTHOF(dest),
                                  nfd.getBuffer(), nfd.length(),
                                  U_FOLD_CASE_DEFAULT, &errorCode);
        return (UBool)(U_SUCCESS(errorCode) &&
                       0!=u_strCompare(nfd.getBuffer(), nfd.length(),
                                       dest, destLength, FALSE));
    }
}
#endif

#if UCONFIG_NO_NORMALIZATION
static UBool changesWhenNFKC_Casefolded(const BinaryProperty &, UChar32, UProperty) {
    return FALSE;
}
#else
static UBool changesWhenNFKC_Casefolded(const BinaryProperty &, UChar32 c, UProperty ) {
    UErrorCode errorCode=U_ZERO_ERROR;
    const Normalizer2Impl *kcf=Normalizer2Factory::getNFKC_CFImpl(errorCode);
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    UnicodeString src(c);
    UnicodeString dest;
    {
        
        
        ReorderingBuffer buffer(*kcf, dest);
        
        if(buffer.init(5, errorCode)) {
            const UChar *srcArray=src.getBuffer();
            kcf->compose(srcArray, srcArray+src.length(), FALSE,
                          TRUE, buffer, errorCode);
        }
    }
    return U_SUCCESS(errorCode) && dest!=src;
}
#endif

#if UCONFIG_NO_NORMALIZATION
static UBool isCanonSegmentStarter(const BinaryProperty &, UChar32, UProperty) {
    return FALSE;
}
#else
static UBool isCanonSegmentStarter(const BinaryProperty &, UChar32 c, UProperty ) {
    UErrorCode errorCode=U_ZERO_ERROR;
    const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(errorCode);
    return
        U_SUCCESS(errorCode) && impl->ensureCanonIterData(errorCode) &&
        impl->isCanonSegmentStarter(c);
}
#endif

static UBool isPOSIX_alnum(const BinaryProperty &, UChar32 c, UProperty ) {
    return u_isalnumPOSIX(c);
}

static UBool isPOSIX_blank(const BinaryProperty &, UChar32 c, UProperty ) {
    return u_isblank(c);
}

static UBool isPOSIX_graph(const BinaryProperty &, UChar32 c, UProperty ) {
    return u_isgraphPOSIX(c);
}

static UBool isPOSIX_print(const BinaryProperty &, UChar32 c, UProperty ) {
    return u_isprintPOSIX(c);
}

static UBool isPOSIX_xdigit(const BinaryProperty &, UChar32 c, UProperty ) {
    return u_isxdigit(c);
}

static const BinaryProperty binProps[UCHAR_BINARY_LIMIT]={
    







    { 1,                U_MASK(UPROPS_ALPHABETIC), defaultContains },
    { 1,                U_MASK(UPROPS_ASCII_HEX_DIGIT), defaultContains },
    { UPROPS_SRC_BIDI,  0, isBidiControl },
    { UPROPS_SRC_BIDI,  0, isMirrored },
    { 1,                U_MASK(UPROPS_DASH), defaultContains },
    { 1,                U_MASK(UPROPS_DEFAULT_IGNORABLE_CODE_POINT), defaultContains },
    { 1,                U_MASK(UPROPS_DEPRECATED), defaultContains },
    { 1,                U_MASK(UPROPS_DIACRITIC), defaultContains },
    { 1,                U_MASK(UPROPS_EXTENDER), defaultContains },
    { UPROPS_SRC_NFC,   0, hasFullCompositionExclusion },
    { 1,                U_MASK(UPROPS_GRAPHEME_BASE), defaultContains },
    { 1,                U_MASK(UPROPS_GRAPHEME_EXTEND), defaultContains },
    { 1,                U_MASK(UPROPS_GRAPHEME_LINK), defaultContains },
    { 1,                U_MASK(UPROPS_HEX_DIGIT), defaultContains },
    { 1,                U_MASK(UPROPS_HYPHEN), defaultContains },
    { 1,                U_MASK(UPROPS_ID_CONTINUE), defaultContains },
    { 1,                U_MASK(UPROPS_ID_START), defaultContains },
    { 1,                U_MASK(UPROPS_IDEOGRAPHIC), defaultContains },
    { 1,                U_MASK(UPROPS_IDS_BINARY_OPERATOR), defaultContains },
    { 1,                U_MASK(UPROPS_IDS_TRINARY_OPERATOR), defaultContains },
    { UPROPS_SRC_BIDI,  0, isJoinControl },
    { 1,                U_MASK(UPROPS_LOGICAL_ORDER_EXCEPTION), defaultContains },
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { 1,                U_MASK(UPROPS_MATH), defaultContains },
    { 1,                U_MASK(UPROPS_NONCHARACTER_CODE_POINT), defaultContains },
    { 1,                U_MASK(UPROPS_QUOTATION_MARK), defaultContains },
    { 1,                U_MASK(UPROPS_RADICAL), defaultContains },
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { 1,                U_MASK(UPROPS_TERMINAL_PUNCTUATION), defaultContains },
    { 1,                U_MASK(UPROPS_UNIFIED_IDEOGRAPH), defaultContains },
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { 1,                U_MASK(UPROPS_WHITE_SPACE), defaultContains },
    { 1,                U_MASK(UPROPS_XID_CONTINUE), defaultContains },
    { 1,                U_MASK(UPROPS_XID_START), defaultContains },
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { 1,                U_MASK(UPROPS_S_TERM), defaultContains },
    { 1,                U_MASK(UPROPS_VARIATION_SELECTOR), defaultContains },
    { UPROPS_SRC_NFC,   0, isNormInert },  
    { UPROPS_SRC_NFKC,  0, isNormInert },  
    { UPROPS_SRC_NFC,   0, isNormInert },  
    { UPROPS_SRC_NFKC,  0, isNormInert },  
    { UPROPS_SRC_NFC_CANON_ITER, 0, isCanonSegmentStarter },
    { 1,                U_MASK(UPROPS_PATTERN_SYNTAX), defaultContains },
    { 1,                U_MASK(UPROPS_PATTERN_WHITE_SPACE), defaultContains },
    { UPROPS_SRC_CHAR_AND_PROPSVEC,  0, isPOSIX_alnum },
    { UPROPS_SRC_CHAR,  0, isPOSIX_blank },
    { UPROPS_SRC_CHAR,  0, isPOSIX_graph },
    { UPROPS_SRC_CHAR,  0, isPOSIX_print },
    { UPROPS_SRC_CHAR,  0, isPOSIX_xdigit },
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { UPROPS_SRC_CASE_AND_NORM,  0, changesWhenCasefolded },
    { UPROPS_SRC_CASE,  0, caseBinaryPropertyContains },  
    { UPROPS_SRC_NFKC_CF, 0, changesWhenNFKC_Casefolded }
};

U_CAPI UBool U_EXPORT2
u_hasBinaryProperty(UChar32 c, UProperty which) {
    
    if(which<UCHAR_BINARY_START || UCHAR_BINARY_LIMIT<=which) {
        
        return FALSE;
    } else {
        const BinaryProperty &prop=binProps[which];
        return prop.contains(prop, c, which);
    }
}

struct IntProperty;

typedef int32_t IntPropertyGetValue(const IntProperty &prop, UChar32 c, UProperty which);
typedef int32_t IntPropertyGetMaxValue(const IntProperty &prop, UProperty which);

struct IntProperty {
    int32_t column;  
    uint32_t mask;
    int32_t shift;  
    IntPropertyGetValue *getValue;
    IntPropertyGetMaxValue *getMaxValue;
};

static int32_t defaultGetValue(const IntProperty &prop, UChar32 c, UProperty ) {
    
    return (int32_t)(u_getUnicodeProperties(c, prop.column)&prop.mask)>>prop.shift;
}

static int32_t defaultGetMaxValue(const IntProperty &prop, UProperty ) {
    return (uprv_getMaxValues(prop.column)&prop.mask)>>prop.shift;
}

static int32_t getMaxValueFromShift(const IntProperty &prop, UProperty ) {
    return prop.shift;
}

static int32_t getBiDiClass(const IntProperty &, UChar32 c, UProperty ) {
    return (int32_t)u_charDirection(c);
}

static int32_t biDiGetMaxValue(const IntProperty &, UProperty which) {
    return ubidi_getMaxValue(GET_BIDI_PROPS(), which);
}

#if UCONFIG_NO_NORMALIZATION
static int32_t getCombiningClass(const IntProperty &, UChar32, UProperty) {
    return 0;
}
#else
static int32_t getCombiningClass(const IntProperty &, UChar32 c, UProperty ) {
    return u_getCombiningClass(c);
}
#endif

static int32_t getGeneralCategory(const IntProperty &, UChar32 c, UProperty ) {
    return (int32_t)u_charType(c);
}

static int32_t getJoiningGroup(const IntProperty &, UChar32 c, UProperty ) {
    return ubidi_getJoiningGroup(GET_BIDI_PROPS(), c);
}

static int32_t getJoiningType(const IntProperty &, UChar32 c, UProperty ) {
    return ubidi_getJoiningType(GET_BIDI_PROPS(), c);
}

static int32_t getNumericType(const IntProperty &, UChar32 c, UProperty ) {
    int32_t ntv=(int32_t)GET_NUMERIC_TYPE_VALUE(u_getMainProperties(c));
    return UPROPS_NTV_GET_TYPE(ntv);
}

static int32_t getScript(const IntProperty &, UChar32 c, UProperty ) {
    UErrorCode errorCode=U_ZERO_ERROR;
    return (int32_t)uscript_getScript(c, &errorCode);
}





static const UHangulSyllableType gcbToHst[]={
    U_HST_NOT_APPLICABLE,   
    U_HST_NOT_APPLICABLE,   
    U_HST_NOT_APPLICABLE,   
    U_HST_NOT_APPLICABLE,   
    U_HST_LEADING_JAMO,     
    U_HST_NOT_APPLICABLE,   
    U_HST_LV_SYLLABLE,      
    U_HST_LVT_SYLLABLE,     
    U_HST_TRAILING_JAMO,    
    U_HST_VOWEL_JAMO        
    



};

static int32_t getHangulSyllableType(const IntProperty &, UChar32 c, UProperty ) {
    
    int32_t gcb=(int32_t)(u_getUnicodeProperties(c, 2)&UPROPS_GCB_MASK)>>UPROPS_GCB_SHIFT;
    if(gcb<LENGTHOF(gcbToHst)) {
        return gcbToHst[gcb];
    } else {
        return U_HST_NOT_APPLICABLE;
    }
}

#if UCONFIG_NO_NORMALIZATION
static int32_t getNormQuickCheck(const IntProperty &, UChar32, UProperty) {
    return 0;
}
#else
static int32_t getNormQuickCheck(const IntProperty &, UChar32 c, UProperty which) {
    return (int32_t)unorm_getQuickCheck(c, (UNormalizationMode)(which-UCHAR_NFD_QUICK_CHECK+UNORM_NFD));
}
#endif

#if UCONFIG_NO_NORMALIZATION
static int32_t getLeadCombiningClass(const IntProperty &, UChar32, UProperty) {
    return 0;
}
#else
static int32_t getLeadCombiningClass(const IntProperty &, UChar32 c, UProperty ) {
    return unorm_getFCD16(c)>>8;
}
#endif

#if UCONFIG_NO_NORMALIZATION
static int32_t getTrailCombiningClass(const IntProperty &, UChar32, UProperty) {
    return 0;
}
#else
static int32_t getTrailCombiningClass(const IntProperty &, UChar32 c, UProperty ) {
    return unorm_getFCD16(c)&0xff;
}
#endif

static const IntProperty intProps[UCHAR_INT_LIMIT-UCHAR_INT_START]={
    







    { UPROPS_SRC_BIDI,  0, 0,                               getBiDiClass, biDiGetMaxValue },
    { 0,                UPROPS_BLOCK_MASK, UPROPS_BLOCK_SHIFT, defaultGetValue, defaultGetMaxValue },
    { UPROPS_SRC_NFC,   0, 0xff,                            getCombiningClass, getMaxValueFromShift },
    { 2,                UPROPS_DT_MASK, 0,                  defaultGetValue, defaultGetMaxValue },
    { 0,                UPROPS_EA_MASK, UPROPS_EA_SHIFT,    defaultGetValue, defaultGetMaxValue },
    { UPROPS_SRC_CHAR,  0, (int32_t)U_CHAR_CATEGORY_COUNT-1,getGeneralCategory, getMaxValueFromShift },
    { UPROPS_SRC_BIDI,  0, 0,                               getJoiningGroup, biDiGetMaxValue },
    { UPROPS_SRC_BIDI,  0, 0,                               getJoiningType, biDiGetMaxValue },
    { 2,                UPROPS_LB_MASK, UPROPS_LB_SHIFT,    defaultGetValue, defaultGetMaxValue },
    { UPROPS_SRC_CHAR,  0, (int32_t)U_NT_COUNT-1,           getNumericType, getMaxValueFromShift },
    { 0,                UPROPS_SCRIPT_MASK, 0,              getScript, defaultGetMaxValue },
    { UPROPS_SRC_PROPSVEC, 0, (int32_t)U_HST_COUNT-1,       getHangulSyllableType, getMaxValueFromShift },
    
    { UPROPS_SRC_NFC,   0, (int32_t)UNORM_YES,              getNormQuickCheck, getMaxValueFromShift },
    
    { UPROPS_SRC_NFKC,  0, (int32_t)UNORM_YES,              getNormQuickCheck, getMaxValueFromShift },
    
    { UPROPS_SRC_NFC,   0, (int32_t)UNORM_MAYBE,            getNormQuickCheck, getMaxValueFromShift },
    
    { UPROPS_SRC_NFKC,  0, (int32_t)UNORM_MAYBE,            getNormQuickCheck, getMaxValueFromShift },
    { UPROPS_SRC_NFC,   0, 0xff,                            getLeadCombiningClass, getMaxValueFromShift },
    { UPROPS_SRC_NFC,   0, 0xff,                            getTrailCombiningClass, getMaxValueFromShift },
    { 2,                UPROPS_GCB_MASK, UPROPS_GCB_SHIFT,  defaultGetValue, defaultGetMaxValue },
    { 2,                UPROPS_SB_MASK, UPROPS_SB_SHIFT,    defaultGetValue, defaultGetMaxValue },
    { 2,                UPROPS_WB_MASK, UPROPS_WB_SHIFT,    defaultGetValue, defaultGetMaxValue }
};

U_CAPI int32_t U_EXPORT2
u_getIntPropertyValue(UChar32 c, UProperty which) {
    if(which<UCHAR_INT_START) {
        if(UCHAR_BINARY_START<=which && which<UCHAR_BINARY_LIMIT) {
            const BinaryProperty &prop=binProps[which];
            return prop.contains(prop, c, which);
        }
    } else if(which<UCHAR_INT_LIMIT) {
        const IntProperty &prop=intProps[which-UCHAR_INT_START];
        return prop.getValue(prop, c, which);
    } else if(which==UCHAR_GENERAL_CATEGORY_MASK) {
        return U_MASK(u_charType(c));
    }
    return 0;  
}

U_CAPI int32_t U_EXPORT2
u_getIntPropertyMinValue(UProperty ) {
    return 0; 
}

U_CAPI int32_t U_EXPORT2
u_getIntPropertyMaxValue(UProperty which) {
    if(which<UCHAR_INT_START) {
        if(UCHAR_BINARY_START<=which && which<UCHAR_BINARY_LIMIT) {
            return 1;  
        }
    } else if(which<UCHAR_INT_LIMIT) {
        const IntProperty &prop=intProps[which-UCHAR_INT_START];
        return prop.getMaxValue(prop, which);
    }
    return -1;  
}

U_CFUNC UPropertySource U_EXPORT2
uprops_getSource(UProperty which) {
    if(which<UCHAR_BINARY_START) {
        return UPROPS_SRC_NONE; 
    } else if(which<UCHAR_BINARY_LIMIT) {
        const BinaryProperty &prop=binProps[which];
        if(prop.mask!=0) {
            return UPROPS_SRC_PROPSVEC;
        } else {
            return (UPropertySource)prop.column;
        }
    } else if(which<UCHAR_INT_START) {
        return UPROPS_SRC_NONE; 
    } else if(which<UCHAR_INT_LIMIT) {
        const IntProperty &prop=intProps[which-UCHAR_INT_START];
        if(prop.mask!=0) {
            return UPROPS_SRC_PROPSVEC;
        } else {
            return (UPropertySource)prop.column;
        }
    } else if(which<UCHAR_STRING_START) {
        switch(which) {
        case UCHAR_GENERAL_CATEGORY_MASK:
        case UCHAR_NUMERIC_VALUE:
            return UPROPS_SRC_CHAR;

        default:
            return UPROPS_SRC_NONE;
        }
    } else if(which<UCHAR_STRING_LIMIT) {
        switch(which) {
        case UCHAR_AGE:
            return UPROPS_SRC_PROPSVEC;

        case UCHAR_BIDI_MIRRORING_GLYPH:
            return UPROPS_SRC_BIDI;

        case UCHAR_CASE_FOLDING:
        case UCHAR_LOWERCASE_MAPPING:
        case UCHAR_SIMPLE_CASE_FOLDING:
        case UCHAR_SIMPLE_LOWERCASE_MAPPING:
        case UCHAR_SIMPLE_TITLECASE_MAPPING:
        case UCHAR_SIMPLE_UPPERCASE_MAPPING:
        case UCHAR_TITLECASE_MAPPING:
        case UCHAR_UPPERCASE_MAPPING:
            return UPROPS_SRC_CASE;

        case UCHAR_ISO_COMMENT:
        case UCHAR_NAME:
        case UCHAR_UNICODE_1_NAME:
            return UPROPS_SRC_NAMES;

        default:
            return UPROPS_SRC_NONE;
        }
    } else {
        switch(which) {
        case UCHAR_SCRIPT_EXTENSIONS:
            return UPROPS_SRC_PROPSVEC;
        default:
            return UPROPS_SRC_NONE; 
        }
    }
}

#if !UCONFIG_NO_NORMALIZATION

U_CAPI int32_t U_EXPORT2
u_getFC_NFKC_Closure(UChar32 c, UChar *dest, int32_t destCapacity, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    
    
    
    
    
    const Normalizer2 *nfkc=Normalizer2Factory::getNFKCInstance(*pErrorCode);
    const UCaseProps *csp=ucase_getSingleton();
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    
    UnicodeString folded1String;
    const UChar *folded1;
    int32_t folded1Length=ucase_toFullFolding(csp, c, &folded1, U_FOLD_CASE_DEFAULT);
    if(folded1Length<0) {
        const Normalizer2Impl *nfkcImpl=Normalizer2Factory::getImpl(nfkc);
        if(nfkcImpl->getCompQuickCheck(nfkcImpl->getNorm16(c))!=UNORM_NO) {
            return u_terminateUChars(dest, destCapacity, 0, pErrorCode);  
        }
        folded1String.setTo(c);
    } else {
        if(folded1Length>UCASE_MAX_STRING_LENGTH) {
            folded1String.setTo(folded1Length);
        } else {
            folded1String.setTo(FALSE, folded1, folded1Length);
        }
    }
    UnicodeString kc1=nfkc->normalize(folded1String, *pErrorCode);
    
    UnicodeString folded2String(kc1);
    UnicodeString kc2=nfkc->normalize(folded2String.foldCase(), *pErrorCode);
    
    if(U_FAILURE(*pErrorCode) || kc1==kc2) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    } else {
        return kc2.extract(dest, destCapacity, *pErrorCode);
    }
}

#endif
