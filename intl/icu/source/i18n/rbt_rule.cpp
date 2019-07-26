









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/rep.h"
#include "unicode/unifilt.h"
#include "unicode/uniset.h"
#include "unicode/utf16.h"
#include "rbt_rule.h"
#include "rbt_data.h"
#include "cmemory.h"
#include "strmatch.h"
#include "strrepl.h"
#include "util.h"
#include "putilimp.h"

static const UChar FORWARD_OP[] = {32,62,32,0}; 

U_NAMESPACE_BEGIN


























TransliterationRule::TransliterationRule(const UnicodeString& input,
                                         int32_t anteContextPos, int32_t postContextPos,
                                         const UnicodeString& outputStr,
                                         int32_t cursorPosition, int32_t cursorOffset,
                                         UnicodeFunctor** segs,
                                         int32_t segsCount,
                                         UBool anchorStart, UBool anchorEnd,
                                         const TransliterationRuleData* theData,
                                         UErrorCode& status) :
    UMemory(),
    segments(0),
    data(theData) {

    if (U_FAILURE(status)) {
        return;
    }
    
    if (anteContextPos < 0) {
        anteContextLength = 0;
    } else {
        if (anteContextPos > input.length()) {
            
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        anteContextLength = anteContextPos;
    }
    if (postContextPos < 0) {
        keyLength = input.length() - anteContextLength;
    } else {
        if (postContextPos < anteContextLength ||
            postContextPos > input.length()) {
            
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        keyLength = postContextPos - anteContextLength;
    }
    if (cursorPosition < 0) {
        cursorPosition = outputStr.length();
    } else if (cursorPosition > outputStr.length()) {
        
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    
    
    
    this->segments = segs;
    this->segmentsCount = segsCount;

    pattern = input;
    flags = 0;
    if (anchorStart) {
        flags |= ANCHOR_START;
    }
    if (anchorEnd) {
        flags |= ANCHOR_END;
    }

    anteContext = NULL;
    if (anteContextLength > 0) {
        anteContext = new StringMatcher(pattern, 0, anteContextLength,
                                        FALSE, *data);
        
        if (anteContext == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }
    
    key = NULL;
    if (keyLength > 0) {
        key = new StringMatcher(pattern, anteContextLength, anteContextLength + keyLength,
                                FALSE, *data);
        
        if (key == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }
    
    int32_t postContextLength = pattern.length() - keyLength - anteContextLength;
    postContext = NULL;
    if (postContextLength > 0) {
        postContext = new StringMatcher(pattern, anteContextLength + keyLength, pattern.length(),
                                        FALSE, *data);
        
        if (postContext == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    this->output = new StringReplacer(outputStr, cursorPosition + cursorOffset, data);
    
    if (this->output == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}




TransliterationRule::TransliterationRule(TransliterationRule& other) :
    UMemory(other),
    anteContext(NULL),
    key(NULL),
    postContext(NULL),
    pattern(other.pattern),
    anteContextLength(other.anteContextLength),
    keyLength(other.keyLength),
    flags(other.flags),
    data(other.data) {

    segments = NULL;
    segmentsCount = 0;
    if (other.segmentsCount > 0) {
        segments = (UnicodeFunctor **)uprv_malloc(other.segmentsCount * sizeof(UnicodeFunctor *));
        uprv_memcpy(segments, other.segments, other.segmentsCount*sizeof(segments[0]));
    }

    if (other.anteContext != NULL) {
        anteContext = (StringMatcher*) other.anteContext->clone();
    }
    if (other.key != NULL) {
        key = (StringMatcher*) other.key->clone();
    }
    if (other.postContext != NULL) {
        postContext = (StringMatcher*) other.postContext->clone();
    }
    output = other.output->clone();
}

TransliterationRule::~TransliterationRule() {
    uprv_free(segments);
    delete anteContext;
    delete key;
    delete postContext;
    delete output;
}










int32_t TransliterationRule::getContextLength(void) const {
    return anteContextLength + ((flags & ANCHOR_START) ? 1 : 0);
}







int16_t TransliterationRule::getIndexValue() const {
    if (anteContextLength == pattern.length()) {
        
        
        return -1;
    }
    UChar32 c = pattern.char32At(anteContextLength);
    return (int16_t)(data->lookupMatcher(c) == NULL ? (c & 0xFF) : -1);
}











UBool TransliterationRule::matchesIndexValue(uint8_t v) const {
    
    
    UnicodeMatcher *m = (key != NULL) ? key : postContext;
    return (m != NULL) ? m->matchesIndexValue(v) : TRUE;
}







UBool TransliterationRule::masks(const TransliterationRule& r2) const {
    
































    




    int32_t len = pattern.length();
    int32_t left = anteContextLength;
    int32_t left2 = r2.anteContextLength;
    int32_t right = len - left;
    int32_t right2 = r2.pattern.length() - left2;
    int32_t cachedCompare = r2.pattern.compare(left2 - left, len, pattern);

    
    

    
    if (left == left2 && right == right2 &&
        keyLength <= r2.keyLength &&
        0 == cachedCompare) {
        
        return (flags == r2.flags) ||
            (!(flags & ANCHOR_START) && !(flags & ANCHOR_END)) ||
            ((r2.flags & ANCHOR_START) && (r2.flags & ANCHOR_END));
    }

    return left <= left2 &&
        (right < right2 ||
         (right == right2 && keyLength <= r2.keyLength)) &&
         (0 == cachedCompare);
}

static inline int32_t posBefore(const Replaceable& str, int32_t pos) {
    return (pos > 0) ?
        pos - U16_LENGTH(str.char32At(pos-1)) :
        pos - 1;
}

static inline int32_t posAfter(const Replaceable& str, int32_t pos) {
    return (pos >= 0 && pos < str.length()) ?
        pos + U16_LENGTH(str.char32At(pos)) :
        pos + 1;
}






















UMatchDegree TransliterationRule::matchAndReplace(Replaceable& text,
                                                  UTransPosition& pos,
                                                  UBool incremental) const {
    
    
    
    
    

    

    
    if (segments != NULL) {
        for (int32_t i=0; i<segmentsCount; ++i) {
            ((StringMatcher*) segments[i])->resetMatch();
        }
    }


    int32_t keyLimit;

    

    
    
    
    int32_t oText; 

    int32_t minOText;

    
    
    
    
    
    int32_t anteLimit = posBefore(text, pos.contextStart);

    UMatchDegree match;

    
    oText = posBefore(text, pos.start);

    if (anteContext != NULL) {
        match = anteContext->matches(text, oText, anteLimit, FALSE);
        if (match != U_MATCH) {
            return U_MISMATCH;
        }
    }

    minOText = posAfter(text, oText);

    
    
    if (((flags & ANCHOR_START) != 0) && oText != anteLimit) {
        return U_MISMATCH;
    }

    
    
    oText = pos.start;

    if (key != NULL) {
        match = key->matches(text, oText, pos.limit, incremental);
        if (match != U_MATCH) {
            return match;
        }
    }

    keyLimit = oText;

    if (postContext != NULL) {
        if (incremental && keyLimit == pos.limit) {
            
            
            
            
            return U_PARTIAL_MATCH;
        }

        match = postContext->matches(text, oText, pos.contextLimit, incremental);
        if (match != U_MATCH) {
            return match;
        }
    }
    
    
    
    if (((flags & ANCHOR_END)) != 0) {
        if (oText != pos.contextLimit) {
            return U_MISMATCH;
        }
        if (incremental) {
            return U_PARTIAL_MATCH;
        }
    }
    
    

    
    

    int32_t newStart;
    int32_t newLength = output->toReplacer()->replace(text, pos.start, keyLimit, newStart);
    int32_t lenDelta = newLength - (keyLimit - pos.start);

    oText += lenDelta;
    pos.limit += lenDelta;
    pos.contextLimit += lenDelta;
    
    pos.start = uprv_max(minOText, uprv_min(uprv_min(oText, pos.limit), newStart));
    return U_MATCH;
}





UnicodeString& TransliterationRule::toRule(UnicodeString& rule,
                                           UBool escapeUnprintable) const {

    
    
    
    UnicodeString str, quoteBuf;

    
    
    UBool emitBraces =
        (anteContext != NULL) || (postContext != NULL);

    
    if ((flags & ANCHOR_START) != 0) {
        rule.append((UChar)94);
    }

    
    ICU_Utility::appendToRule(rule, anteContext, escapeUnprintable, quoteBuf);

    if (emitBraces) {
        ICU_Utility::appendToRule(rule, (UChar) 0x007B , TRUE, escapeUnprintable, quoteBuf);
    }

    ICU_Utility::appendToRule(rule, key, escapeUnprintable, quoteBuf);

    if (emitBraces) {
        ICU_Utility::appendToRule(rule, (UChar) 0x007D , TRUE, escapeUnprintable, quoteBuf);
    }

    ICU_Utility::appendToRule(rule, postContext, escapeUnprintable, quoteBuf);

    
    if ((flags & ANCHOR_END) != 0) {
        rule.append((UChar)36);
    }

    ICU_Utility::appendToRule(rule, UnicodeString(TRUE, FORWARD_OP, 3), TRUE, escapeUnprintable, quoteBuf);

    

    ICU_Utility::appendToRule(rule, output->toReplacer()->toReplacerPattern(str, escapeUnprintable),
                              TRUE, escapeUnprintable, quoteBuf);

    ICU_Utility::appendToRule(rule, (UChar) 0x003B , TRUE, escapeUnprintable, quoteBuf);

    return rule;
}

void TransliterationRule::setData(const TransliterationRuleData* d) {
    data = d;
    if (anteContext != NULL) anteContext->setData(d);
    if (postContext != NULL) postContext->setData(d);
    if (key != NULL) key->setData(d);
    
    output->setData(d);
    
}





void TransliterationRule::addSourceSetTo(UnicodeSet& toUnionTo) const {
    int32_t limit = anteContextLength + keyLength;
    for (int32_t i=anteContextLength; i<limit; ) {
        UChar32 ch = pattern.char32At(i);
        i += U16_LENGTH(ch);
        const UnicodeMatcher* matcher = data->lookupMatcher(ch);
        if (matcher == NULL) {
            toUnionTo.add(ch);
        } else {
            matcher->addMatchSetTo(toUnionTo);
        }
    }
}





void TransliterationRule::addTargetSetTo(UnicodeSet& toUnionTo) const {
    output->toReplacer()->addReplacementSetTo(toUnionTo);
}

U_NAMESPACE_END

#endif 


