









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/utf16.h"
#include "rbt_set.h"
#include "rbt_rule.h"
#include "cmemory.h"
#include "putilimp.h"

U_CDECL_BEGIN
static void U_CALLCONV _deleteRule(void *rule) {
    delete (icu::TransliterationRule *)rule;
}
U_CDECL_END







#ifdef DEBUG_RBT
#include <stdio.h>
#include "charstr.h"






static UnicodeString& _formatInput(UnicodeString &appendTo,
                                   const UnicodeString& input,
                                   const UTransPosition& pos) {
    
    
    
    if (0 <= pos.contextStart &&
        pos.contextStart <= pos.start &&
        pos.start <= pos.limit &&
        pos.limit <= pos.contextLimit &&
        pos.contextLimit <= input.length()) {

        UnicodeString a, b, c, d, e;
        input.extractBetween(0, pos.contextStart, a);
        input.extractBetween(pos.contextStart, pos.start, b);
        input.extractBetween(pos.start, pos.limit, c);
        input.extractBetween(pos.limit, pos.contextLimit, d);
        input.extractBetween(pos.contextLimit, input.length(), e);
        appendTo.append(a).append((UChar)123).append(b).
            append((UChar)124).append(c).append((UChar)124).append(d).
            append((UChar)125).append(e);
    } else {
        appendTo.append("INVALID UTransPosition");
        
        
        
        
    }
    return appendTo;
}


UnicodeString& _appendHex(uint32_t number,
                          int32_t digits,
                          UnicodeString& target) {
    static const UChar digitString[] = {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0
    };
    while (digits--) {
        target += digitString[(number >> (digits*4)) & 0xF];
    }
    return target;
}


UnicodeString& _escape(const UnicodeString &source,
                       UnicodeString &target) {
    for (int32_t i = 0; i < source.length(); ) {
        UChar32 ch = source.char32At(i);
        i += U16_LENGTH(ch);
        if (ch < 0x09 || (ch > 0x0A && ch < 0x20)|| ch > 0x7E) {
            if (ch <= 0xFFFF) {
                target += "\\u";
                _appendHex(ch, 4, target);
            } else {
                target += "\\U";
                _appendHex(ch, 8, target);
            }
        } else {
            target += ch;
        }
    }
    return target;
}

inline void _debugOut(const char* msg, TransliterationRule* rule,
                      const Replaceable& theText, UTransPosition& pos) {
    UnicodeString buf(msg, "");
    if (rule) {
        UnicodeString r;
        rule->toRule(r, TRUE);
        buf.append((UChar)32).append(r);
    }
    buf.append(UnicodeString(" => ", ""));
    UnicodeString* text = (UnicodeString*)&theText;
    _formatInput(buf, *text, pos);
    UnicodeString esc;
    _escape(buf, esc);
    CharString cbuf(esc);
    printf("%s\n", (const char*) cbuf);
}

#else
#define _debugOut(msg, rule, theText, pos)
#endif







static void maskingError(const icu::TransliterationRule& rule1,
                         const icu::TransliterationRule& rule2,
                         UParseError& parseError) {
    icu::UnicodeString r;
    int32_t len;

    parseError.line = parseError.offset = -1;
    
    
    rule1.toRule(r, FALSE);
    len = uprv_min(r.length(), U_PARSE_CONTEXT_LEN-1);
    r.extract(0, len, parseError.preContext);
    parseError.preContext[len] = 0;   
    
    
    r.truncate(0);
    rule2.toRule(r, FALSE);
    len = uprv_min(r.length(), U_PARSE_CONTEXT_LEN-1);
    r.extract(0, len, parseError.postContext);
    parseError.postContext[len] = 0;   
}

U_NAMESPACE_BEGIN




TransliterationRuleSet::TransliterationRuleSet(UErrorCode& status) : UMemory() {
    ruleVector = new UVector(&_deleteRule, NULL, status);
    if (U_FAILURE(status)) {
        return;
    }
    if (ruleVector == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    rules = NULL;
    maxContextLength = 0;
}




TransliterationRuleSet::TransliterationRuleSet(const TransliterationRuleSet& other) :
    UMemory(other),
    ruleVector(0),
    rules(0),
    maxContextLength(other.maxContextLength) {

    int32_t i, len;
    uprv_memcpy(index, other.index, sizeof(index));
    UErrorCode status = U_ZERO_ERROR;
    ruleVector = new UVector(&_deleteRule, NULL, status);
    if (other.ruleVector != 0 && ruleVector != 0 && U_SUCCESS(status)) {
        len = other.ruleVector->size();
        for (i=0; i<len && U_SUCCESS(status); ++i) {
            TransliterationRule *tempTranslitRule = new TransliterationRule(*(TransliterationRule*)other.ruleVector->elementAt(i));
            
            if (tempTranslitRule == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            ruleVector->addElement(tempTranslitRule, status);
            if (U_FAILURE(status)) {
                break;
            }
        }
    }
    if (other.rules != 0 && U_SUCCESS(status)) {
        UParseError p;
        freeze(p, status);
    }
}




TransliterationRuleSet::~TransliterationRuleSet() {
    delete ruleVector; 
    uprv_free(rules);
}

void TransliterationRuleSet::setData(const TransliterationRuleData* d) {
    


    int32_t len = index[256]; 
    for (int32_t i=0; i<len; ++i) {
        rules[i]->setData(d);
    }
}





int32_t TransliterationRuleSet::getMaximumContextLength(void) const {
    return maxContextLength;
}











void TransliterationRuleSet::addRule(TransliterationRule* adoptedRule,
                                     UErrorCode& status) {
    if (U_FAILURE(status)) {
        delete adoptedRule;
        return;
    }
    ruleVector->addElement(adoptedRule, status);

    int32_t len;
    if ((len = adoptedRule->getContextLength()) > maxContextLength) {
        maxContextLength = len;
    }

    uprv_free(rules);
    rules = 0;
}











void TransliterationRuleSet::freeze(UParseError& parseError,UErrorCode& status) {
    















    int32_t n = ruleVector->size();
    int32_t j;
    int16_t x;
    UVector v(2*n, status); 

    if (U_FAILURE(status)) {
        return;
    }

    


    int16_t* indexValue = (int16_t*) uprv_malloc( sizeof(int16_t) * (n > 0 ? n : 1) );
    
    if (indexValue == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    for (j=0; j<n; ++j) {
        TransliterationRule* r = (TransliterationRule*) ruleVector->elementAt(j);
        indexValue[j] = r->getIndexValue();
    }
    for (x=0; x<256; ++x) {
        index[x] = v.size();
        for (j=0; j<n; ++j) {
            if (indexValue[j] >= 0) {
                if (indexValue[j] == x) {
                    v.addElement(ruleVector->elementAt(j), status);
                }
            } else {
                
                
                
                
                TransliterationRule* r = (TransliterationRule*) ruleVector->elementAt(j);
                if (r->matchesIndexValue((uint8_t)x)) {
                    v.addElement(r, status);
                }
            }
        }
    }
    uprv_free(indexValue);
    index[256] = v.size();

    

    uprv_free(rules); 

    
    if (v.size() == 0) {
        rules = NULL;
        return;
    }
    rules = (TransliterationRule **)uprv_malloc(v.size() * sizeof(TransliterationRule *));
    
    if (rules == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    for (j=0; j<v.size(); ++j) {
        rules[j] = (TransliterationRule*) v.elementAt(j);
    }

    
    
    

    






    for (x=0; x<256; ++x) {
        for (j=index[x]; j<index[x+1]-1; ++j) {
            TransliterationRule* r1 = rules[j];
            for (int32_t k=j+1; k<index[x+1]; ++k) {
                TransliterationRule* r2 = rules[k];
                if (r1->masks(*r2)) {






                    status = U_RULE_MASK_ERROR;
                    maskingError(*r1, *r2, parseError);
                    return;
                }
            }
        }
    }

    
    
    
}














UBool TransliterationRuleSet::transliterate(Replaceable& text,
                                            UTransPosition& pos,
                                            UBool incremental) {
    int16_t indexByte = (int16_t) (text.char32At(pos.start) & 0xFF);
    for (int32_t i=index[indexByte]; i<index[indexByte+1]; ++i) {
        UMatchDegree m = rules[i]->matchAndReplace(text, pos, incremental);
        switch (m) {
        case U_MATCH:
            _debugOut("match", rules[i], text, pos);
            return TRUE;
        case U_PARTIAL_MATCH:
            _debugOut("partial match", rules[i], text, pos);
            return FALSE;
        default: 
            break;
        }
    }
    
    pos.start += U16_LENGTH(text.char32At(pos.start));
    _debugOut("no match", NULL, text, pos);
    return TRUE;
}




UnicodeString& TransliterationRuleSet::toRules(UnicodeString& ruleSource,
                                               UBool escapeUnprintable) const {
    int32_t i;
    int32_t count = ruleVector->size();
    ruleSource.truncate(0);
    for (i=0; i<count; ++i) {
        if (i != 0) {
            ruleSource.append((UChar) 0x000A );
        }
        TransliterationRule *r =
            (TransliterationRule*) ruleVector->elementAt(i);
        r->toRule(ruleSource, escapeUnprintable);
    }
    return ruleSource;
}





UnicodeSet& TransliterationRuleSet::getSourceTargetSet(UnicodeSet& result,
                               UBool getTarget) const
{
    result.clear();
    int32_t count = ruleVector->size();
    for (int32_t i=0; i<count; ++i) {
        TransliterationRule* r =
            (TransliterationRule*) ruleVector->elementAt(i);
        if (getTarget) {
            r->addTargetSetTo(result);
        } else {
            r->addSourceSetTo(result);
        }
    }
    return result;
}

U_NAMESPACE_END

#endif 
