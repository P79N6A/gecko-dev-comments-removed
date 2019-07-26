




















#include "unicode/brkiter.h"
#include "unicode/locid.h"
#include "unicode/parsepos.h"
#include "unicode/uniset.h"
#include "cmemory.h"
#include "ruleiter.h"
#include "ucase.h"
#include "util.h"
#include "uvector.h"



#define START_EXTRA 16

U_NAMESPACE_BEGIN




#define _dbgct(me)





UnicodeSet::UnicodeSet(const UnicodeString& pattern,
                       uint32_t options,
                       const SymbolTable* symbols,
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
            applyPattern(pattern, options, symbols, status);
        }
    }
    _dbgct(this);
}

UnicodeSet::UnicodeSet(const UnicodeString& pattern, ParsePosition& pos,
                       uint32_t options,
                       const SymbolTable* symbols,
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
            applyPattern(pattern, pos, options, symbols, status);
        }
    }
    _dbgct(this);
}





UnicodeSet& UnicodeSet::applyPattern(const UnicodeString& pattern,
                                     uint32_t options,
                                     const SymbolTable* symbols,
                                     UErrorCode& status) {
    ParsePosition pos(0);
    applyPattern(pattern, pos, options, symbols, status);
    if (U_FAILURE(status)) return *this;

    int32_t i = pos.getIndex();

    if (options & USET_IGNORE_SPACE) {
        
        ICU_Utility::skipWhitespace(pattern, i, TRUE);
    }

    if (i != pattern.length()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return *this;
}

UnicodeSet& UnicodeSet::applyPattern(const UnicodeString& pattern,
                              ParsePosition& pos,
                              uint32_t options,
                              const SymbolTable* symbols,
                              UErrorCode& status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    if (isFrozen()) {
        status = U_NO_WRITE_PERMISSION;
        return *this;
    }
    
    
    UnicodeString rebuiltPat;
    RuleCharacterIterator chars(pattern, symbols, pos);
    applyPattern(chars, symbols, rebuiltPat, options, &UnicodeSet::closeOver, status);
    if (U_FAILURE(status)) return *this;
    if (chars.inVariable()) {
        
        status = U_MALFORMED_SET;
        return *this;
    }
    setPattern(rebuiltPat);
    return *this;
}



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







static inline void
addCaseMapping(UnicodeSet &set, int32_t result, const UChar *full, UnicodeString &str) {
    if(result >= 0) {
        if(result > UCASE_MAX_STRING_LENGTH) {
            
            set.add(result);
        } else {
            
            str.setTo((UBool)FALSE, full, result);
            set.add(str);
        }
    }
    
    
}

UnicodeSet& UnicodeSet::closeOver(int32_t attribute) {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    if (attribute & (USET_CASE_INSENSITIVE | USET_ADD_CASE_MAPPINGS)) {
        const UCaseProps *csp = ucase_getSingleton();
        {
            UnicodeSet foldSet(*this);
            UnicodeString str;
            USetAdder sa = {
                foldSet.toUSet(),
                _set_add,
                _set_addRange,
                _set_addString,
                NULL, 
                NULL 
            };

            
            
            
            if (attribute & USET_CASE_INSENSITIVE) {
                foldSet.strings->removeAllElements();
            }

            int32_t n = getRangeCount();
            UChar32 result;
            const UChar *full;
            int32_t locCache = 0;

            for (int32_t i=0; i<n; ++i) {
                UChar32 start = getRangeStart(i);
                UChar32 end   = getRangeEnd(i);

                if (attribute & USET_CASE_INSENSITIVE) {
                    
                    for (UChar32 cp=start; cp<=end; ++cp) {
                        ucase_addCaseClosure(csp, cp, &sa);
                    }
                } else {
                    
                    
                    for (UChar32 cp=start; cp<=end; ++cp) {
                        result = ucase_toFullLower(csp, cp, NULL, NULL, &full, "", &locCache);
                        addCaseMapping(foldSet, result, full, str);

                        result = ucase_toFullTitle(csp, cp, NULL, NULL, &full, "", &locCache);
                        addCaseMapping(foldSet, result, full, str);

                        result = ucase_toFullUpper(csp, cp, NULL, NULL, &full, "", &locCache);
                        addCaseMapping(foldSet, result, full, str);

                        result = ucase_toFullFolding(csp, cp, &full, 0);
                        addCaseMapping(foldSet, result, full, str);
                    }
                }
            }
            if (strings != NULL && strings->size() > 0) {
                if (attribute & USET_CASE_INSENSITIVE) {
                    for (int32_t j=0; j<strings->size(); ++j) {
                        str = *(const UnicodeString *) strings->elementAt(j);
                        str.foldCase();
                        if(!ucase_addStringCaseClosure(csp, str.getBuffer(), str.length(), &sa)) {
                            foldSet.add(str); 
                        }
                    }
                } else {
                    Locale root("");
#if !UCONFIG_NO_BREAK_ITERATION
                    UErrorCode status = U_ZERO_ERROR;
                    BreakIterator *bi = BreakIterator::createWordInstance(root, status);
                    if (U_SUCCESS(status)) {
#endif
                        const UnicodeString *pStr;

                        for (int32_t j=0; j<strings->size(); ++j) {
                            pStr = (const UnicodeString *) strings->elementAt(j);
                            (str = *pStr).toLower(root);
                            foldSet.add(str);
#if !UCONFIG_NO_BREAK_ITERATION
                            (str = *pStr).toTitle(bi, root);
                            foldSet.add(str);
#endif
                            (str = *pStr).toUpper(root);
                            foldSet.add(str);
                            (str = *pStr).foldCase();
                            foldSet.add(str);
                        }
#if !UCONFIG_NO_BREAK_ITERATION
                    }
                    delete bi;
#endif
                }
            }
            *this = foldSet;
        }
    }
    return *this;
}

U_NAMESPACE_END
