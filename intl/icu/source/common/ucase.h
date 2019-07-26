

















#ifndef __UCASE_H__
#define __UCASE_H__

#include "unicode/utypes.h"
#include "unicode/uset.h"
#include "putilimp.h"
#include "uset_imp.h"
#include "udataswp.h"

#ifdef __cplusplus
U_NAMESPACE_BEGIN

class UnicodeString;

U_NAMESPACE_END
#endif



U_CDECL_BEGIN

struct UCaseProps;
typedef struct UCaseProps UCaseProps;

U_CDECL_END

U_CAPI const UCaseProps * U_EXPORT2
ucase_getSingleton(void);

U_CFUNC void U_EXPORT2
ucase_addPropertyStarts(const UCaseProps *csp, const USetAdder *sa, UErrorCode *pErrorCode);






U_CFUNC int32_t
ucase_getCaseLocale(const char *locale, int32_t *locCache);


enum {
    UCASE_LOC_UNKNOWN,
    UCASE_LOC_ROOT,
    UCASE_LOC_TURKISH,
    UCASE_LOC_LITHUANIAN,
    UCASE_LOC_DUTCH
};







#define _STRCASECMP_OPTIONS_MASK 0xffff







#define _FOLD_CASE_OPTIONS_MASK 0xff



U_CAPI UChar32 U_EXPORT2
ucase_tolower(const UCaseProps *csp, UChar32 c);

U_CAPI UChar32 U_EXPORT2
ucase_toupper(const UCaseProps *csp, UChar32 c);

U_CAPI UChar32 U_EXPORT2
ucase_totitle(const UCaseProps *csp, UChar32 c);

U_CAPI UChar32 U_EXPORT2
ucase_fold(const UCaseProps *csp, UChar32 c, uint32_t options);










U_CFUNC void U_EXPORT2
ucase_addCaseClosure(const UCaseProps *csp, UChar32 c, const USetAdder *sa);













U_CFUNC UBool U_EXPORT2
ucase_addStringCaseClosure(const UCaseProps *csp, const UChar *s, int32_t length, const USetAdder *sa);

#ifdef __cplusplus
U_NAMESPACE_BEGIN




class U_COMMON_API FullCaseFoldingIterator {
public:
    
    FullCaseFoldingIterator();
    



    UChar32 next(UnicodeString &full);
private:
    FullCaseFoldingIterator(const FullCaseFoldingIterator &);  
    FullCaseFoldingIterator &operator=(const FullCaseFoldingIterator &);  

    const UChar *unfold;
    int32_t unfoldRows;
    int32_t unfoldRowWidth;
    int32_t unfoldStringWidth;
    int32_t currentRow;
    int32_t rowCpIndex;
};

U_NAMESPACE_END
#endif


U_CAPI int32_t U_EXPORT2
ucase_getType(const UCaseProps *csp, UChar32 c);


U_CAPI int32_t U_EXPORT2
ucase_getTypeOrIgnorable(const UCaseProps *csp, UChar32 c);

U_CAPI UBool U_EXPORT2
ucase_isSoftDotted(const UCaseProps *csp, UChar32 c);

U_CAPI UBool U_EXPORT2
ucase_isCaseSensitive(const UCaseProps *csp, UChar32 c);



U_CDECL_BEGIN



















typedef UChar32 U_CALLCONV
UCaseContextIterator(void *context, int8_t dir);





struct UCaseContext {
    void *p;
    int32_t start, index, limit;
    int32_t cpStart, cpLimit;
    int8_t dir;
    int8_t b1, b2, b3;
};
typedef struct UCaseContext UCaseContext;

U_CDECL_END

#define UCASECONTEXT_INITIALIZER { NULL,  0, 0, 0,  0, 0,  0,  0, 0, 0 }

enum {
    













    UCASE_MAX_STRING_LENGTH=0x1f
};






















U_CAPI int32_t U_EXPORT2
ucase_toFullLower(const UCaseProps *csp, UChar32 c,
                  UCaseContextIterator *iter, void *context,
                  const UChar **pString,
                  const char *locale, int32_t *locCache);

U_CAPI int32_t U_EXPORT2
ucase_toFullUpper(const UCaseProps *csp, UChar32 c,
                  UCaseContextIterator *iter, void *context,
                  const UChar **pString,
                  const char *locale, int32_t *locCache);

U_CAPI int32_t U_EXPORT2
ucase_toFullTitle(const UCaseProps *csp, UChar32 c,
                  UCaseContextIterator *iter, void *context,
                  const UChar **pString,
                  const char *locale, int32_t *locCache);

U_CAPI int32_t U_EXPORT2
ucase_toFullFolding(const UCaseProps *csp, UChar32 c,
                    const UChar **pString,
                    uint32_t options);

U_CFUNC int32_t U_EXPORT2
ucase_hasBinaryProperty(UChar32 c, UProperty which);


U_CDECL_BEGIN




typedef int32_t U_CALLCONV
UCaseMapFull(const UCaseProps *csp, UChar32 c,
             UCaseContextIterator *iter, void *context,
             const UChar **pString,
             const char *locale, int32_t *locCache);

U_CDECL_END



#define UCASE_DATA_NAME "ucase"
#define UCASE_DATA_TYPE "icu"


#define UCASE_FMT_0 0x63
#define UCASE_FMT_1 0x41
#define UCASE_FMT_2 0x53
#define UCASE_FMT_3 0x45


enum {
    UCASE_IX_INDEX_TOP,
    UCASE_IX_LENGTH,
    UCASE_IX_TRIE_SIZE,
    UCASE_IX_EXC_LENGTH,
    UCASE_IX_UNFOLD_LENGTH,

    UCASE_IX_MAX_FULL_LENGTH=15,
    UCASE_IX_TOP=16
};




#define UCASE_TYPE_MASK     3
enum {
    UCASE_NONE,
    UCASE_LOWER,
    UCASE_UPPER,
    UCASE_TITLE
};

#define UCASE_GET_TYPE(props) ((props)&UCASE_TYPE_MASK)
#define UCASE_GET_TYPE_AND_IGNORABLE(props) ((props)&7)

#define UCASE_IGNORABLE         4
#define UCASE_SENSITIVE         8
#define UCASE_EXCEPTION         0x10

#define UCASE_DOT_MASK      0x60
enum {
    UCASE_NO_DOT=0,         
    UCASE_SOFT_DOTTED=0x20, 
    UCASE_ABOVE=0x40,       
    UCASE_OTHER_ACCENT=0x60 
};


#define UCASE_DELTA_SHIFT   7
#define UCASE_DELTA_MASK    0xff80
#define UCASE_MAX_DELTA     0xff
#define UCASE_MIN_DELTA     (-UCASE_MAX_DELTA-1)

#if U_SIGNED_RIGHT_SHIFT_IS_ARITHMETIC
#   define UCASE_GET_DELTA(props) ((int16_t)(props)>>UCASE_DELTA_SHIFT)
#else
#   define UCASE_GET_DELTA(props) (int16_t)(((props)&0x8000) ? (((props)>>UCASE_DELTA_SHIFT)|0xfe00) : ((uint16_t)(props)>>UCASE_DELTA_SHIFT))
#endif


#define UCASE_EXC_SHIFT     5
#define UCASE_EXC_MASK      0xffe0
#define UCASE_MAX_EXCEPTIONS ((UCASE_EXC_MASK>>UCASE_EXC_SHIFT)+1)




enum {
    UCASE_EXC_LOWER,
    UCASE_EXC_FOLD,
    UCASE_EXC_UPPER,
    UCASE_EXC_TITLE,
    UCASE_EXC_4,            
    UCASE_EXC_5,            
    UCASE_EXC_CLOSURE,
    UCASE_EXC_FULL_MAPPINGS,
    UCASE_EXC_ALL_SLOTS     
};


#define UCASE_EXC_DOUBLE_SLOTS      0x100




#define UCASE_EXC_DOT_SHIFT     7


#define UCASE_EXC_DOT_MASK      0x3000
enum {
    UCASE_EXC_NO_DOT=0,
    UCASE_EXC_SOFT_DOTTED=0x1000,
    UCASE_EXC_ABOVE=0x2000,         
    UCASE_EXC_OTHER_ACCENT=0x3000   
};


#define UCASE_EXC_CONDITIONAL_SPECIAL   0x4000
#define UCASE_EXC_CONDITIONAL_FOLD      0x8000


#define UCASE_FULL_LOWER    0xf
#define UCASE_FULL_FOLDING  0xf0
#define UCASE_FULL_UPPER    0xf00
#define UCASE_FULL_TITLE    0xf000


#define UCASE_FULL_MAPPINGS_MAX_LENGTH (4*0xf)
#define UCASE_CLOSURE_MAX_LENGTH 0xf


enum {
    UCASE_UNFOLD_ROWS,
    UCASE_UNFOLD_ROW_WIDTH,
    UCASE_UNFOLD_STRING_WIDTH
};

#endif
