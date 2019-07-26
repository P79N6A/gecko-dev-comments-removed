









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "strmatch.h"
#include "rbt_data.h"
#include "util.h"
#include "unicode/uniset.h"
#include "unicode/utf16.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(StringMatcher)

StringMatcher::StringMatcher(const UnicodeString& theString,
                             int32_t start,
                             int32_t limit,
                             int32_t segmentNum,
                             const TransliterationRuleData& theData) :
    data(&theData),
    segmentNumber(segmentNum),
    matchStart(-1),
    matchLimit(-1)
{
    theString.extractBetween(start, limit, pattern);
}

StringMatcher::StringMatcher(const StringMatcher& o) :
    UnicodeFunctor(o),
    UnicodeMatcher(o),
    UnicodeReplacer(o),
    pattern(o.pattern),
    data(o.data),
    segmentNumber(o.segmentNumber),
    matchStart(o.matchStart),
    matchLimit(o.matchLimit)
{
}




StringMatcher::~StringMatcher() {
}




UnicodeFunctor* StringMatcher::clone() const {
    return new StringMatcher(*this);
}





UnicodeMatcher* StringMatcher::toMatcher() const {
  StringMatcher  *nonconst_this = const_cast<StringMatcher *>(this);
  UnicodeMatcher *nonconst_base = static_cast<UnicodeMatcher *>(nonconst_this);
  
  return nonconst_base;
}





UnicodeReplacer* StringMatcher::toReplacer() const {
  StringMatcher  *nonconst_this = const_cast<StringMatcher *>(this);
  UnicodeReplacer *nonconst_base = static_cast<UnicodeReplacer *>(nonconst_this);
  
  return nonconst_base;
}




UMatchDegree StringMatcher::matches(const Replaceable& text,
                                    int32_t& offset,
                                    int32_t limit,
                                    UBool incremental) {
    int32_t i;
    int32_t cursor = offset;
    if (limit < cursor) {
        
        for (i=pattern.length()-1; i>=0; --i) {
            UChar keyChar = pattern.charAt(i);
            UnicodeMatcher* subm = data->lookupMatcher(keyChar);
            if (subm == 0) {
                if (cursor > limit &&
                    keyChar == text.charAt(cursor)) {
                    --cursor;
                } else {
                    return U_MISMATCH;
                }
            } else {
                UMatchDegree m =
                    subm->matches(text, cursor, limit, incremental);
                if (m != U_MATCH) {
                    return m;
                }
            }
        }
        
        
        
        if (matchStart < 0) {
            matchStart = cursor+1;
            matchLimit = offset+1;
        }
    } else {
        for (i=0; i<pattern.length(); ++i) {
            if (incremental && cursor == limit) {
                
                
                return U_PARTIAL_MATCH;
            }
            UChar keyChar = pattern.charAt(i);
            UnicodeMatcher* subm = data->lookupMatcher(keyChar);
            if (subm == 0) {
                
                
                
                if (cursor < limit &&
                    keyChar == text.charAt(cursor)) {
                    ++cursor;
                } else {
                    return U_MISMATCH;
                }
            } else {
                UMatchDegree m =
                    subm->matches(text, cursor, limit, incremental);
                if (m != U_MATCH) {
                    return m;
                }
            }
        }
        
        matchStart = offset;
        matchLimit = cursor;
    }

    offset = cursor;
    return U_MATCH;
}




UnicodeString& StringMatcher::toPattern(UnicodeString& result,
                                        UBool escapeUnprintable) const
{
    result.truncate(0);
    UnicodeString str, quoteBuf;
    if (segmentNumber > 0) {
        result.append((UChar)40); 
    }
    for (int32_t i=0; i<pattern.length(); ++i) {
        UChar keyChar = pattern.charAt(i);
        const UnicodeMatcher* m = data->lookupMatcher(keyChar);
        if (m == 0) {
            ICU_Utility::appendToRule(result, keyChar, FALSE, escapeUnprintable, quoteBuf);
        } else {
            ICU_Utility::appendToRule(result, m->toPattern(str, escapeUnprintable),
                         TRUE, escapeUnprintable, quoteBuf);
        }
    }
    if (segmentNumber > 0) {
        result.append((UChar)41); 
    }
    
    ICU_Utility::appendToRule(result, -1,
                              TRUE, escapeUnprintable, quoteBuf);
    return result;
}




UBool StringMatcher::matchesIndexValue(uint8_t v) const {
    if (pattern.length() == 0) {
        return TRUE;
    }
    UChar32 c = pattern.char32At(0);
    const UnicodeMatcher *m = data->lookupMatcher(c);
    return (m == 0) ? ((c & 0xFF) == v) : m->matchesIndexValue(v);
}




void StringMatcher::addMatchSetTo(UnicodeSet& toUnionTo) const {
    UChar32 ch;
    for (int32_t i=0; i<pattern.length(); i+=U16_LENGTH(ch)) {
        ch = pattern.char32At(i);
        const UnicodeMatcher* matcher = data->lookupMatcher(ch);
        if (matcher == NULL) {
            toUnionTo.add(ch);
        } else {
            matcher->addMatchSetTo(toUnionTo);
        }
    }
}




int32_t StringMatcher::replace(Replaceable& text,
                               int32_t start,
                               int32_t limit,
                               int32_t& ) {
    
    int32_t outLen = 0;
    
    
    int32_t dest = limit;
    
    
    if (matchStart >= 0) {
        if (matchStart != matchLimit) {
            text.copy(matchStart, matchLimit, dest);
            outLen = matchLimit - matchStart;
        }
    }
    
    text.handleReplaceBetween(start, limit, UnicodeString()); 
    
    return outLen;
}




UnicodeString& StringMatcher::toReplacerPattern(UnicodeString& rule,
                                                UBool ) const {
    
    rule.truncate(0);
    rule.append((UChar)0x0024 );
    ICU_Utility::appendNumber(rule, segmentNumber, 10, 1);
    return rule;
}





 void StringMatcher::resetMatch() {
    matchStart = matchLimit = -1;
}






void StringMatcher::addReplacementSetTo(UnicodeSet& ) const {
    
    
    
    
    
}




void StringMatcher::setData(const TransliterationRuleData* d) {
    data = d;
    int32_t i = 0;
    while (i<pattern.length()) {
        UChar32 c = pattern.char32At(i);
        UnicodeFunctor* f = data->lookup(c);
        if (f != NULL) {
            f->setData(data);
        }
        i += U16_LENGTH(c);
    }
}

U_NAMESPACE_END

#endif 


