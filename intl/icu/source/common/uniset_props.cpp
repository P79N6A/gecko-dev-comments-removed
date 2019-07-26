

















#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/parsepos.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "unicode/symtable.h"
#include "unicode/uset.h"
#include "unicode/locid.h"
#include "unicode/brkiter.h"
#include "uset_imp.h"
#include "ruleiter.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "util.h"
#include "uvector.h"
#include "uprops.h"
#include "propname.h"
#include "normalizer2impl.h"
#include "ucase.h"
#include "ubidi_props.h"
#include "uinvchar.h"
#include "uprops.h"
#include "charstr.h"
#include "cstring.h"
#include "mutex.h"
#include "umutex.h"
#include "uassert.h"
#include "hash.h"

U_NAMESPACE_USE

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))



#define START_EXTRA 16



#define SET_OPEN        ((UChar)0x005B) /*[*/
#define SET_CLOSE       ((UChar)0x005D) /*]*/
#define HYPHEN          ((UChar)0x002D) /*-*/
#define COMPLEMENT      ((UChar)0x005E) /*^*/
#define COLON           ((UChar)0x003A) /*:*/
#define BACKSLASH       ((UChar)0x005C) /*\*/
#define INTERSECTION    ((UChar)0x0026) /*&*/
#define UPPER_U         ((UChar)0x0055) /*U*/
#define LOWER_U         ((UChar)0x0075) /*u*/
#define OPEN_BRACE      ((UChar)123)    /*{*/
#define CLOSE_BRACE     ((UChar)125)    /*}*/
#define UPPER_P         ((UChar)0x0050) /*P*/
#define LOWER_P         ((UChar)0x0070) /*p*/
#define UPPER_N         ((UChar)78)     /*N*/
#define EQUALS          ((UChar)0x003D) /*=*/


static const UChar POSIX_CLOSE[] = { COLON,SET_CLOSE,0 };  



static const UChar HYPHEN_RIGHT_BRACE[] = {HYPHEN,SET_CLOSE,0}; 


static const char ANY[]   = "ANY";   
static const char ASCII[] = "ASCII"; 
static const char ASSIGNED[] = "Assigned"; 


#define NAME_PROP "na"
#define NAME_PROP_LENGTH 2









U_CDECL_BEGIN
static UBool U_CALLCONV uset_cleanup();
U_CDECL_END




class UnicodeSetSingleton : public SimpleSingletonWrapper<UnicodeSet> {
public:
    UnicodeSetSingleton(SimpleSingleton &s, const char *pattern) :
            SimpleSingletonWrapper<UnicodeSet>(s), fPattern(pattern) {}
    UnicodeSet *getInstance(UErrorCode &errorCode) {
        return SimpleSingletonWrapper<UnicodeSet>::getInstance(createInstance, fPattern, errorCode);
    }
private:
    static void *createInstance(const void *context, UErrorCode &errorCode) {
        UnicodeString pattern((const char *)context, -1, US_INV);
        UnicodeSet *set=new UnicodeSet(pattern, errorCode);
        if(set==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        set->freeze();
        ucln_common_registerCleanup(UCLN_COMMON_USET, uset_cleanup);
        return set;
    }

    const char *fPattern;
};

U_CDECL_BEGIN

static UnicodeSet *INCLUSIONS[UPROPS_SRC_COUNT] = { NULL }; 

STATIC_SIMPLE_SINGLETON(uni32Singleton);







static void U_CALLCONV
_set_add(USet *set, UChar32 c) {
    ((UnicodeSet *)set)->add(c);
}

static void U_CALLCONV
_set_addRange(USet *set, UChar32 start, UChar32 end) {
    ((UnicodeSet *)set)->add(start, end);
}

static void U_CALLCONV
_set_addString(USet *set, const UChar *str, int32_t length) {
    ((UnicodeSet *)set)->add(UnicodeString((UBool)(length<0), str, length));
}




static UBool U_CALLCONV uset_cleanup(void) {
    int32_t i;

    for(i = UPROPS_SRC_NONE; i < UPROPS_SRC_COUNT; ++i) {
        if (INCLUSIONS[i] != NULL) {
            delete INCLUSIONS[i];
            INCLUSIONS[i] = NULL;
        }
    }
    UnicodeSetSingleton(uni32Singleton, NULL).deleteInstance();
    return TRUE;
}

U_CDECL_END

U_NAMESPACE_BEGIN






#define DEFAULT_INCLUSION_CAPACITY 3072

const UnicodeSet* UnicodeSet::getInclusions(int32_t src, UErrorCode &status) {
    UBool needInit;
    UMTX_CHECK(NULL, (INCLUSIONS[src] == NULL), needInit);
    if (needInit) {
        UnicodeSet* incl = new UnicodeSet();
        USetAdder sa = {
            (USet *)incl,
            _set_add,
            _set_addRange,
            _set_addString,
            NULL, 
            NULL 
        };
        if (incl != NULL) {
            incl->ensureCapacity(DEFAULT_INCLUSION_CAPACITY, status);
            switch(src) {
            case UPROPS_SRC_CHAR:
                uchar_addPropertyStarts(&sa, &status);
                break;
            case UPROPS_SRC_PROPSVEC:
                upropsvec_addPropertyStarts(&sa, &status);
                break;
            case UPROPS_SRC_CHAR_AND_PROPSVEC:
                uchar_addPropertyStarts(&sa, &status);
                upropsvec_addPropertyStarts(&sa, &status);
                break;
#if !UCONFIG_NO_NORMALIZATION
            case UPROPS_SRC_CASE_AND_NORM: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                ucase_addPropertyStarts(ucase_getSingleton(), &sa, &status);
                break;
            }
            case UPROPS_SRC_NFC: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                break;
            }
            case UPROPS_SRC_NFKC: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFKCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                break;
            }
            case UPROPS_SRC_NFKC_CF: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFKC_CFImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                break;
            }
            case UPROPS_SRC_NFC_CANON_ITER: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addCanonIterPropertyStarts(&sa, status);
                }
                break;
            }
#endif
            case UPROPS_SRC_CASE:
                ucase_addPropertyStarts(ucase_getSingleton(), &sa, &status);
                break;
            case UPROPS_SRC_BIDI:
                ubidi_addPropertyStarts(ubidi_getSingleton(), &sa, &status);
                break;
            default:
                status = U_INTERNAL_PROGRAM_ERROR;
                break;
            }
            if (U_SUCCESS(status)) {
                
                incl->compact();
                umtx_lock(NULL);
                if (INCLUSIONS[src] == NULL) {
                    INCLUSIONS[src] = incl;
                    incl = NULL;
                    ucln_common_registerCleanup(UCLN_COMMON_USET, uset_cleanup);
                }
                umtx_unlock(NULL);
            }
            delete incl;
        } else {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return INCLUSIONS[src];
}



U_CFUNC UnicodeSet *
uniset_getUnicode32Instance(UErrorCode &errorCode) {
    return UnicodeSetSingleton(uni32Singleton, "[:age=3.2:]").getInstance(errorCode);
}








static inline UBool
isPerlOpen(const UnicodeString &pattern, int32_t pos) {
    UChar c;
    return pattern.charAt(pos)==BACKSLASH && ((c=pattern.charAt(pos+1))==LOWER_P || c==UPPER_P);
}






static inline UBool
isNameOpen(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==BACKSLASH && pattern.charAt(pos+1)==UPPER_N;
}

static inline UBool
isPOSIXOpen(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==SET_OPEN && pattern.charAt(pos+1)==COLON;
}









#define _dbgct(me)











UnicodeSet::UnicodeSet(const UnicodeString& pattern,
                       UErrorCode& status) :
    len(0), capacity(START_EXTRA), list(0), bmpSet(0), buffer(0),
    bufferCapacity(0), patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{
    if(U_SUCCESS(status)){
        list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
        
        if(list == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;  
        }else{
            allocateStrings(status);
            applyPattern(pattern, status);
        }
    }
    _dbgct(this);
}





UnicodeSet& UnicodeSet::applyPattern(const UnicodeString& pattern,
                                     UErrorCode& status) {
    
    
    
    ParsePosition pos(0);
    applyPatternIgnoreSpace(pattern, pos, NULL, status);
    if (U_FAILURE(status)) return *this;

    int32_t i = pos.getIndex();
    
    ICU_Utility::skipWhitespace(pattern, i, TRUE);
    if (i != pattern.length()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return *this;
}

void
UnicodeSet::applyPatternIgnoreSpace(const UnicodeString& pattern,
                                    ParsePosition& pos,
                                    const SymbolTable* symbols,
                                    UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (isFrozen()) {
        status = U_NO_WRITE_PERMISSION;
        return;
    }
    
    
    UnicodeString rebuiltPat;
    RuleCharacterIterator chars(pattern, symbols, pos);
    applyPattern(chars, symbols, rebuiltPat, USET_IGNORE_SPACE, NULL, status);
    if (U_FAILURE(status)) return;
    if (chars.inVariable()) {
        
        status = U_MALFORMED_SET;
        return;
    }
    setPattern(rebuiltPat);
}





UBool UnicodeSet::resemblesPattern(const UnicodeString& pattern, int32_t pos) {
    return ((pos+1) < pattern.length() &&
            pattern.charAt(pos) == (UChar)91) ||
        resemblesPropertyPattern(pattern, pos);
}









class UnicodeSetPointer {
    UnicodeSet* p;
public:
    inline UnicodeSetPointer() : p(0) {}
    inline ~UnicodeSetPointer() { delete p; }
    inline UnicodeSet* pointer() { return p; }
    inline UBool allocate() {
        if (p == 0) {
            p = new UnicodeSet();
        }
        return p != 0;
    }
};















void UnicodeSet::applyPattern(RuleCharacterIterator& chars,
                              const SymbolTable* symbols,
                              UnicodeString& rebuiltPat,
                              uint32_t options,
                              UnicodeSet& (UnicodeSet::*caseClosure)(int32_t attribute),
                              UErrorCode& ec) {
    if (U_FAILURE(ec)) return;

    

    

    int32_t opts = RuleCharacterIterator::PARSE_VARIABLES |
                   RuleCharacterIterator::PARSE_ESCAPES;
    if ((options & USET_IGNORE_SPACE) != 0) {
        opts |= RuleCharacterIterator::SKIP_WHITESPACE;
    }

    UnicodeString patLocal, buf;
    UBool usePat = FALSE;
    UnicodeSetPointer scratch;
    RuleCharacterIterator::Pos backup;

    
    
    int8_t lastItem = 0, mode = 0;
    UChar32 lastChar = 0;
    UChar op = 0;

    UBool invert = FALSE;

    clear();

    while (mode != 2 && !chars.atEnd()) {
        U_ASSERT((lastItem == 0 && op == 0) ||
                 (lastItem == 1 && (op == 0 || op == HYPHEN )) ||
                 (lastItem == 2 && (op == 0 || op == HYPHEN  ||
                                    op == INTERSECTION )));

        UChar32 c = 0;
        UBool literal = FALSE;
        UnicodeSet* nested = 0; 

        

        
        int8_t setMode = 0;
        if (resemblesPropertyPattern(chars, opts)) {
            setMode = 2;
        }

        
        
        
        
        
        
        

        else {
            
            chars.getPos(backup);
            c = chars.next(opts, literal, ec);
            if (U_FAILURE(ec)) return;

            if (c == 0x5B  && !literal) {
                if (mode == 1) {
                    chars.setPos(backup); 
                    setMode = 1;
                } else {
                    
                    mode = 1;
                    patLocal.append((UChar) 0x5B );
                    chars.getPos(backup); 
                    c = chars.next(opts, literal, ec); 
                    if (U_FAILURE(ec)) return;
                    if (c == 0x5E  && !literal) {
                        invert = TRUE;
                        patLocal.append((UChar) 0x5E );
                        chars.getPos(backup); 
                        c = chars.next(opts, literal, ec);
                        if (U_FAILURE(ec)) return;
                    }
                    
                    
                    if (c == HYPHEN ) {
                        literal = TRUE;
                        
                    } else {
                        chars.setPos(backup); 
                        continue;
                    }
                }
            } else if (symbols != 0) {
                const UnicodeFunctor *m = symbols->lookupMatcher(c);
                if (m != 0) {
                    const UnicodeSet *ms = dynamic_cast<const UnicodeSet *>(m);
                    if (ms == NULL) {
                        ec = U_MALFORMED_SET;
                        return;
                    }
                    
                    
                    nested = const_cast<UnicodeSet*>(ms);
                    setMode = 3;
                }
            }
        }

        
        
        
        

        if (setMode != 0) {
            if (lastItem == 1) {
                if (op != 0) {
                    
                    ec = U_MALFORMED_SET;
                    return;
                }
                add(lastChar, lastChar);
                _appendToPat(patLocal, lastChar, FALSE);
                lastItem = 0;
                op = 0;
            }

            if (op == HYPHEN  || op == INTERSECTION ) {
                patLocal.append(op);
            }

            if (nested == 0) {
                
                if (!scratch.allocate()) {
                    ec = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                nested = scratch.pointer();
            }
            switch (setMode) {
            case 1:
                nested->applyPattern(chars, symbols, patLocal, options, caseClosure, ec);
                break;
            case 2:
                chars.skipIgnored(opts);
                nested->applyPropertyPattern(chars, patLocal, ec);
                if (U_FAILURE(ec)) return;
                break;
            case 3: 
                nested->_toPattern(patLocal, FALSE);
                break;
            }

            usePat = TRUE;

            if (mode == 0) {
                
                *this = *nested;
                mode = 2;
                break;
            }

            switch (op) {
            case HYPHEN: 
                removeAll(*nested);
                break;
            case INTERSECTION: 
                retainAll(*nested);
                break;
            case 0:
                addAll(*nested);
                break;
            }

            op = 0;
            lastItem = 2;

            continue;
        }

        if (mode == 0) {
            
            ec = U_MALFORMED_SET;
            return;
        }

        
        
        

        if (!literal) {
            switch (c) {
            case 0x5D :
                if (lastItem == 1) {
                    add(lastChar, lastChar);
                    _appendToPat(patLocal, lastChar, FALSE);
                }
                
                if (op == HYPHEN ) {
                    add(op, op);
                    patLocal.append(op);
                } else if (op == INTERSECTION ) {
                    
                    ec = U_MALFORMED_SET;
                    return;
                }
                patLocal.append((UChar) 0x5D );
                mode = 2;
                continue;
            case HYPHEN :
                if (op == 0) {
                    if (lastItem != 0) {
                        op = (UChar) c;
                        continue;
                    } else {
                        
                        add(c, c);
                        c = chars.next(opts, literal, ec);
                        if (U_FAILURE(ec)) return;
                        if (c == 0x5D  && !literal) {
                            patLocal.append(HYPHEN_RIGHT_BRACE, 2);
                            mode = 2;
                            continue;
                        }
                    }
                }
                
                ec = U_MALFORMED_SET;
                return;
            case INTERSECTION :
                if (lastItem == 2 && op == 0) {
                    op = (UChar) c;
                    continue;
                }
                
                ec = U_MALFORMED_SET;
                return;
            case 0x5E :
                
                ec = U_MALFORMED_SET;
                return;
            case 0x7B :
                if (op != 0) {
                    
                    ec = U_MALFORMED_SET;
                    return;
                }
                if (lastItem == 1) {
                    add(lastChar, lastChar);
                    _appendToPat(patLocal, lastChar, FALSE);
                }
                lastItem = 0;
                buf.truncate(0);
                {
                    UBool ok = FALSE;
                    while (!chars.atEnd()) {
                        c = chars.next(opts, literal, ec);
                        if (U_FAILURE(ec)) return;
                        if (c == 0x7D  && !literal) {
                            ok = TRUE;
                            break;
                        }
                        buf.append(c);
                    }
                    if (buf.length() < 1 || !ok) {
                        
                        ec = U_MALFORMED_SET;
                        return;
                    }
                }
                
                
                
                add(buf);
                patLocal.append((UChar) 0x7B );
                _appendToPat(patLocal, buf, FALSE);
                patLocal.append((UChar) 0x7D );
                continue;
            case SymbolTable::SYMBOL_REF:
                
                
                
                
                
                
                {
                    chars.getPos(backup);
                    c = chars.next(opts, literal, ec);
                    if (U_FAILURE(ec)) return;
                    UBool anchor = (c == 0x5D  && !literal);
                    if (symbols == 0 && !anchor) {
                        c = SymbolTable::SYMBOL_REF;
                        chars.setPos(backup);
                        break; 
                    }
                    if (anchor && op == 0) {
                        if (lastItem == 1) {
                            add(lastChar, lastChar);
                            _appendToPat(patLocal, lastChar, FALSE);
                        }
                        add(U_ETHER);
                        usePat = TRUE;
                        patLocal.append((UChar) SymbolTable::SYMBOL_REF);
                        patLocal.append((UChar) 0x5D );
                        mode = 2;
                        continue;
                    }
                    
                    ec = U_MALFORMED_SET;
                    return;
                }
            default:
                break;
            }
        }

        
        
        

        switch (lastItem) {
        case 0:
            lastItem = 1;
            lastChar = c;
            break;
        case 1:
            if (op == HYPHEN ) {
                if (lastChar >= c) {
                    
                    
                    
                    ec = U_MALFORMED_SET;
                    return;
                }
                add(lastChar, c);
                _appendToPat(patLocal, lastChar, FALSE);
                patLocal.append(op);
                _appendToPat(patLocal, c, FALSE);
                lastItem = 0;
                op = 0;
            } else {
                add(lastChar, lastChar);
                _appendToPat(patLocal, lastChar, FALSE);
                lastChar = c;
            }
            break;
        case 2:
            if (op != 0) {
                
                ec = U_MALFORMED_SET;
                return;
            }
            lastChar = c;
            lastItem = 1;
            break;
        }
    }

    if (mode != 2) {
        
        ec = U_MALFORMED_SET;
        return;
    }

    chars.skipIgnored(opts);

    





    if ((options & USET_CASE_INSENSITIVE) != 0) {
        (this->*caseClosure)(USET_CASE_INSENSITIVE);
    }
    else if ((options & USET_ADD_CASE_MAPPINGS) != 0) {
        (this->*caseClosure)(USET_ADD_CASE_MAPPINGS);
    }
    if (invert) {
        complement();
    }

    
    
    if (usePat) {
        rebuiltPat.append(patLocal);
    } else {
        _generatePattern(rebuiltPat, FALSE);
    }
    if (isBogus() && U_SUCCESS(ec)) {
        
        ec = U_MEMORY_ALLOCATION_ERROR;
    }
}





static UBool numericValueFilter(UChar32 ch, void* context) {
    return u_getNumericValue(ch) == *(double*)context;
}

static UBool generalCategoryMaskFilter(UChar32 ch, void* context) {
    int32_t value = *(int32_t*)context;
    return (U_GET_GC_MASK((UChar32) ch) & value) != 0;
}

static UBool versionFilter(UChar32 ch, void* context) {
    static const UVersionInfo none = { 0, 0, 0, 0 };
    UVersionInfo v;
    u_charAge(ch, v);
    UVersionInfo* version = (UVersionInfo*)context;
    return uprv_memcmp(&v, &none, sizeof(v)) > 0 && uprv_memcmp(&v, version, sizeof(v)) <= 0;
}

typedef struct {
    UProperty prop;
    int32_t value;
} IntPropertyContext;

static UBool intPropertyFilter(UChar32 ch, void* context) {
    IntPropertyContext* c = (IntPropertyContext*)context;
    return u_getIntPropertyValue((UChar32) ch, c->prop) == c->value;
}

static UBool scriptExtensionsFilter(UChar32 ch, void* context) {
    return uscript_hasScript(ch, *(UScriptCode*)context);
}




void UnicodeSet::applyFilter(UnicodeSet::Filter filter,
                             void* context,
                             int32_t src,
                             UErrorCode &status) {
    if (U_FAILURE(status)) return;

    
    
    
    
    
    
    
    
    
    const UnicodeSet* inclusions = getInclusions(src, status);
    if (U_FAILURE(status)) {
        return;
    }

    clear();

    UChar32 startHasProperty = -1;
    int32_t limitRange = inclusions->getRangeCount();

    for (int j=0; j<limitRange; ++j) {
        
        UChar32 start = inclusions->getRangeStart(j);
        UChar32 end = inclusions->getRangeEnd(j);

        
        for (UChar32 ch = start; ch <= end; ++ch) {
            
            
            if ((*filter)(ch, context)) {
                if (startHasProperty < 0) {
                    startHasProperty = ch;
                }
            } else if (startHasProperty >= 0) {
                add(startHasProperty, ch-1);
                startHasProperty = -1;
            }
        }
    }
    if (startHasProperty >= 0) {
        add((UChar32)startHasProperty, (UChar32)0x10FFFF);
    }
    if (isBogus() && U_SUCCESS(status)) {
        
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

static UBool mungeCharName(char* dst, const char* src, int32_t dstCapacity) {
    
    int32_t j = 0;
    char ch;
    --dstCapacity; 
    while ((ch = *src++) != 0) {
        if (ch == ' ' && (j==0 || (j>0 && dst[j-1]==' '))) {
            continue;
        }
        if (j >= dstCapacity) return FALSE;
        dst[j++] = ch;
    }
    if (j > 0 && dst[j-1] == ' ') --j;
    dst[j] = 0;
    return TRUE;
}





#define FAIL(ec) {ec=U_ILLEGAL_ARGUMENT_ERROR; return *this;}

UnicodeSet&
UnicodeSet::applyIntPropertyValue(UProperty prop, int32_t value, UErrorCode& ec) {
    if (U_FAILURE(ec) || isFrozen()) return *this;

    if (prop == UCHAR_GENERAL_CATEGORY_MASK) {
        applyFilter(generalCategoryMaskFilter, &value, UPROPS_SRC_CHAR, ec);
    } else if (prop == UCHAR_SCRIPT_EXTENSIONS) {
        UScriptCode script = (UScriptCode)value;
        applyFilter(scriptExtensionsFilter, &script, UPROPS_SRC_PROPSVEC, ec);
    } else {
        IntPropertyContext c = {prop, value};
        applyFilter(intPropertyFilter, &c, uprops_getSource(prop), ec);
    }
    return *this;
}

UnicodeSet&
UnicodeSet::applyPropertyAlias(const UnicodeString& prop,
                               const UnicodeString& value,
                               UErrorCode& ec) {
    if (U_FAILURE(ec) || isFrozen()) return *this;

    
    
    
    
    
    
    if( !uprv_isInvariantUString(prop.getBuffer(), prop.length()) ||
        !uprv_isInvariantUString(value.getBuffer(), value.length())
    ) {
        FAIL(ec);
    }
    CharString pname, vname;
    pname.appendInvariantChars(prop, ec);
    vname.appendInvariantChars(value, ec);
    if (U_FAILURE(ec)) return *this;

    UProperty p;
    int32_t v;
    UBool mustNotBeEmpty = FALSE, invert = FALSE;

    if (value.length() > 0) {
        p = u_getPropertyEnum(pname.data());
        if (p == UCHAR_INVALID_CODE) FAIL(ec);

        
        if (p == UCHAR_GENERAL_CATEGORY) {
            p = UCHAR_GENERAL_CATEGORY_MASK;
        }

        if ((p >= UCHAR_BINARY_START && p < UCHAR_BINARY_LIMIT) ||
            (p >= UCHAR_INT_START && p < UCHAR_INT_LIMIT) ||
            (p >= UCHAR_MASK_START && p < UCHAR_MASK_LIMIT)) {
            v = u_getPropertyValueEnum(p, vname.data());
            if (v == UCHAR_INVALID_CODE) {
                
                if (p == UCHAR_CANONICAL_COMBINING_CLASS ||
                    p == UCHAR_TRAIL_CANONICAL_COMBINING_CLASS ||
                    p == UCHAR_LEAD_CANONICAL_COMBINING_CLASS) {
                    char* end;
                    double value = uprv_strtod(vname.data(), &end);
                    v = (int32_t) value;
                    if (v != value || v < 0 || *end != 0) {
                        
                        FAIL(ec);
                    }
                    
                    
                    mustNotBeEmpty = TRUE;
                } else {
                    FAIL(ec);
                }
            }
        }

        else {

            switch (p) {
            case UCHAR_NUMERIC_VALUE:
                {
                    char* end;
                    double value = uprv_strtod(vname.data(), &end);
                    if (*end != 0) {
                        FAIL(ec);
                    }
                    applyFilter(numericValueFilter, &value, UPROPS_SRC_CHAR, ec);
                    return *this;
                }
            case UCHAR_NAME:
                {
                    
                    
                    char buf[128]; 
                    if (!mungeCharName(buf, vname.data(), sizeof(buf))) FAIL(ec);
                    UChar32 ch = u_charFromName(U_EXTENDED_CHAR_NAME, buf, &ec);
                    if (U_SUCCESS(ec)) {
                        clear();
                        add(ch);
                        return *this;
                    } else {
                        FAIL(ec);
                    }
                }
            case UCHAR_UNICODE_1_NAME:
                
                FAIL(ec);
            case UCHAR_AGE:
                {
                    
                    
                    char buf[128];
                    if (!mungeCharName(buf, vname.data(), sizeof(buf))) FAIL(ec);
                    UVersionInfo version;
                    u_versionFromString(version, buf);
                    applyFilter(versionFilter, &version, UPROPS_SRC_PROPSVEC, ec);
                    return *this;
                }
            case UCHAR_SCRIPT_EXTENSIONS:
                v = u_getPropertyValueEnum(UCHAR_SCRIPT, vname.data());
                if (v == UCHAR_INVALID_CODE) {
                    FAIL(ec);
                }
                
                break;
            default:
                
                
                FAIL(ec);
            }
        }
    }

    else {
        
        
        p = UCHAR_GENERAL_CATEGORY_MASK;
        v = u_getPropertyValueEnum(p, pname.data());
        if (v == UCHAR_INVALID_CODE) {
            p = UCHAR_SCRIPT;
            v = u_getPropertyValueEnum(p, pname.data());
            if (v == UCHAR_INVALID_CODE) {
                p = u_getPropertyEnum(pname.data());
                if (p >= UCHAR_BINARY_START && p < UCHAR_BINARY_LIMIT) {
                    v = 1;
                } else if (0 == uprv_comparePropertyNames(ANY, pname.data())) {
                    set(MIN_VALUE, MAX_VALUE);
                    return *this;
                } else if (0 == uprv_comparePropertyNames(ASCII, pname.data())) {
                    set(0, 0x7F);
                    return *this;
                } else if (0 == uprv_comparePropertyNames(ASSIGNED, pname.data())) {
                    
                    p = UCHAR_GENERAL_CATEGORY_MASK;
                    v = U_GC_CN_MASK;
                    invert = TRUE;
                } else {
                    FAIL(ec);
                }
            }
        }
    }

    applyIntPropertyValue(p, v, ec);
    if(invert) {
        complement();
    }

    if (U_SUCCESS(ec) && (mustNotBeEmpty && isEmpty())) {
        
        
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }

    if (isBogus() && U_SUCCESS(ec)) {
        
        ec = U_MEMORY_ALLOCATION_ERROR;
    }
    return *this;
}









UBool UnicodeSet::resemblesPropertyPattern(const UnicodeString& pattern,
                                           int32_t pos) {
    
    if ((pos+5) > pattern.length()) {
        return FALSE;
    }

    
    return isPOSIXOpen(pattern, pos) || isPerlOpen(pattern, pos) || isNameOpen(pattern, pos);
}









UBool UnicodeSet::resemblesPropertyPattern(RuleCharacterIterator& chars,
                                           int32_t iterOpts) {
    
    UBool result = FALSE, literal;
    UErrorCode ec = U_ZERO_ERROR;
    iterOpts &= ~RuleCharacterIterator::PARSE_ESCAPES;
    RuleCharacterIterator::Pos pos;
    chars.getPos(pos);
    UChar32 c = chars.next(iterOpts, literal, ec);
    if (c == 0x5B  || c == 0x5C ) {
        UChar32 d = chars.next(iterOpts & ~RuleCharacterIterator::SKIP_WHITESPACE,
                               literal, ec);
        result = (c == 0x5B ) ? (d == 0x3A ) :
                 (d == 0x4E  || d == 0x70  || d == 0x50 );
    }
    chars.setPos(pos);
    return result && U_SUCCESS(ec);
}




UnicodeSet& UnicodeSet::applyPropertyPattern(const UnicodeString& pattern,
                                             ParsePosition& ppos,
                                             UErrorCode &ec) {
    int32_t pos = ppos.getIndex();

    UBool posix = FALSE; 
    UBool isName = FALSE; 
    UBool invert = FALSE;

    if (U_FAILURE(ec)) return *this;

    
    if ((pos+5) > pattern.length()) {
        FAIL(ec);
    }

    
    
    if (isPOSIXOpen(pattern, pos)) {
        posix = TRUE;
        pos += 2;
        pos = ICU_Utility::skipWhitespace(pattern, pos);
        if (pos < pattern.length() && pattern.charAt(pos) == COMPLEMENT) {
            ++pos;
            invert = TRUE;
        }
    } else if (isPerlOpen(pattern, pos) || isNameOpen(pattern, pos)) {
        UChar c = pattern.charAt(pos+1);
        invert = (c == UPPER_P);
        isName = (c == UPPER_N);
        pos += 2;
        pos = ICU_Utility::skipWhitespace(pattern, pos);
        if (pos == pattern.length() || pattern.charAt(pos++) != OPEN_BRACE) {
            
            FAIL(ec);
        }
    } else {
        
        FAIL(ec);
    }

    
    int32_t close;
    if (posix) {
      close = pattern.indexOf(POSIX_CLOSE, 2, pos);
    } else {
      close = pattern.indexOf(CLOSE_BRACE, pos);
    }
    if (close < 0) {
        
        FAIL(ec);
    }

    
    
    
    int32_t equals = pattern.indexOf(EQUALS, pos);
    UnicodeString propName, valueName;
    if (equals >= 0 && equals < close && !isName) {
        
        pattern.extractBetween(pos, equals, propName);
        pattern.extractBetween(equals+1, close, valueName);
    }

    else {
        
        pattern.extractBetween(pos, close, propName);
            
        
        if (isName) {
            
            
            
            
            
            valueName = propName;
            propName = UnicodeString(NAME_PROP, NAME_PROP_LENGTH, US_INV);
        }
    }

    applyPropertyAlias(propName, valueName, ec);

    if (U_SUCCESS(ec)) {
        if (invert) {
            complement();
        }
            
        
        
        ppos.setIndex(close + (posix ? 2 : 1));
    }

    return *this;
}










void UnicodeSet::applyPropertyPattern(RuleCharacterIterator& chars,
                                      UnicodeString& rebuiltPat,
                                      UErrorCode& ec) {
    if (U_FAILURE(ec)) return;
    UnicodeString pattern;
    chars.lookahead(pattern);
    ParsePosition pos(0);
    applyPropertyPattern(pattern, pos, ec);
    if (U_FAILURE(ec)) return;
    if (pos.getIndex() == 0) {
        
        ec = U_MALFORMED_SET;
        return;
    }
    chars.jumpahead(pos.getIndex());
    rebuiltPat.append(pattern, 0, pos.getIndex());
}

U_NAMESPACE_END
